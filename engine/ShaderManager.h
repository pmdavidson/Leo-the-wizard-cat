#pragma once

#include <vector>
#include <string>
#include <unordered_map>

#include <SFML/Graphics/VertexArray.hpp>
#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/Graphics/Shader.hpp>

#include "SystemManager.h"
#include "Scene.h"
#include "SpriteComponent.h"
#include "LocationComponent.h"
#include "SpriteManager.h"
#include "WindowManager.h"
#include "ShaderManager.h"
#include "MathUtil.h"

namespace ECSEngine
{

/**
 * @brief SpriteSystem
 *
 * Draws all SpriteComponents using:
 *  - A single atlas texture (SpriteManager)
 *  - Batched vertex arrays (1 draw call per shader group)
 *  - Parallax layers
 *  - World-space or screen-space
 *  - Optional per-sprite shader
 *
 * Parallax:
 *  layerParallax[0] = 1.0f
 *  layerParallax[-5] = 0.5f
 *  layerParallax[+5] = 1.4f
 */
template <typename... Components>
class SpriteSystem : public System<Components...>
{
public:
    SpriteSystem() = default;
    ~SpriteSystem() = default;

    //---------------------------------------------------------------------
    // Shader functions (stored inside SpriteSystem)
    //---------------------------------------------------------------------
    void RegisterShader(const std::string &shaderPath, const std::string &shaderName)
    {
        if (mShaders.find(shaderName) != mShaders.end())
            return;

        auto shader = std::make_unique<sf::Shader>();

        if (!shader->loadFromFile(shaderPath, sf::Shader::Type::Fragment))
        {
            std::cerr << "Failed to load shader: " << shaderPath << "\n";
            return;
        }

        mShaders.emplace(shaderName, std::move(shader));
    }

    sf::Shader *GetShader(const std::string &shaderName)
    {
        auto it = mShaders.find(shaderName);
        return (it == mShaders.end()) ? nullptr : it->second.get();
    }

    //---------------------------------------------------------------------
    // Parallax
    //---------------------------------------------------------------------
    void SetParallaxFactor(int layer, float factor)
    {
        mParallaxFactors[layer] = factor;
    }

    float GetParallaxFactor(int layer) const
    {
        auto it = mParallaxFactors.find(layer);
        return (it != mParallaxFactors.end()) ? it->second : 1.0f;
    }

    //---------------------------------------------------------------------
    // Main renderer
    //---------------------------------------------------------------------
    bool Run(Scene<Components...> &scene) override
    {
        auto &entityManager = scene.GetEntityManager();
        auto &spriteManager = scene.GetSpriteManager();
        auto &windowManager = scene.GetWindowManager();

        sf::RenderWindow *window = windowManager.GetWindow();
        if (!window)
            return false;

        sf::Texture &atlasTexture = spriteManager.GetTexture();

        //-----------------------------------------------------------------
        // Step 1: Gather draw items
        //-----------------------------------------------------------------
        struct DrawItem
        {
            int layer = 0;
            float parallax = 1.0f;
            std::string shaderName;

            Rect dest;   // screen-space final rect
            sf::IntRect   texRect; // atlas UV rect
        };

        std::vector<DrawItem> drawItems;
        drawItems.reserve(256);

        for (auto it = entityManager.begin(); it != entityManager.end(); ++it)
        {
            if (!it->isActive())
                continue;

            EntityID id = it->getID();
            if (!entityManager.template HasComponent<SpriteComponent>(id))
                continue;

            auto &spriteComp = entityManager.template GetComponent<SpriteComponent>(id);

            //-----------------------------------------
            // Compute screen-space draw rectangle
            //-----------------------------------------
            Rect screenRect;

            if (spriteComp.inWorldSpace)
            {
                if (!entityManager.template HasComponent<LocationComponent>(id))
                    continue;

                auto &loc = entityManager.template GetComponent<LocationComponent>(id);

                Rect worldRect(
                    loc.position + spriteComp.bounds.topLeft,
                    spriteComp.bounds.width,
                    spriteComp.bounds.height);

                screenRect = windowManager.WorldToWindow(worldRect);
            }
            else
            {
                screenRect = spriteComp.bounds;
            }

            //-----------------------------------------
            // Get UV rect from atlas
            //-----------------------------------------
            sf::Sprite &sfSprite = spriteManager.GetSprite(spriteComp.spriteId);
            sf::IntRect texRect = sfSprite.getTextureRect();

            //-----------------------------------------
            // Build DrawItem
            //-----------------------------------------
            DrawItem item;
            item.layer = spriteComp.layer;
            item.parallax = GetParallaxFactor(spriteComp.layer);
            item.shaderName = spriteComp.shaderName;

            item.dest = screenRect;

            item.texRect = texRect;

            drawItems.push_back(item);
        }

        if (drawItems.empty())
            return true;

        //-----------------------------------------------------------------
        // Step 2: Sort by (layer, shaderName)
        //-----------------------------------------------------------------
        std::sort(drawItems.begin(), drawItems.end(),
                  [](const DrawItem &a, const DrawItem &b)
                  {
                      if (a.layer == b.layer)
                          return a.shaderName < b.shaderName;
                      return a.layer < b.layer;
                  });

        //-----------------------------------------------------------------
        // Step 3: Batch per shader group
        //-----------------------------------------------------------------
        std::size_t i = 0;
        while (i < drawItems.size())
        {
            const std::string &currentShaderName = drawItems[i].shaderName;

            sf::Shader *shader = nullptr;
            if (!currentShaderName.empty())
                shader = GetShader(currentShaderName);

            sf::VertexArray vertices(sf::Triangles);

            //-----------------------------------------------------------------
            // Add all sprites using this shader in one batch
            //-----------------------------------------------------------------
            for (; i < drawItems.size() && drawItems[i].shaderName == currentShaderName; ++i)
            {
                const DrawItem &item = drawItems[i];

                float x = item.dest.topLeft.x;
                float y = item.dest.topLeft.y;
                float w = item.dest.width;
                float h = item.dest.height;

                // Apply parallax
                x *= item.parallax;

                float u0 = static_cast<float>(item.texRect.position.x);
                float v0 = static_cast<float>(item.texRect.position.y);
                float u1 = u0 + static_cast<float>(item.texRect.size.x);
                float v1 = v0 + static_cast<float>(item.texRect.size.y);

                // Triangle 1
                vertices.append(sf::Vertex({x,     y},     {u0, v0}));
                vertices.append(sf::Vertex({x + w, y},     {u1, v0}));
                vertices.append(sf::Vertex({x + w, y + h}, {u1, v1}));

                // Triangle 2
                vertices.append(sf::Vertex({x,     y},     {u0, v0}));
                vertices.append(sf::Vertex({x + w, y + h}, {u1, v1}));
                vertices.append(sf::Vertex({x,     y + h}, {u0, v1}));
            }

            //-----------------------------------------------------------------
            // Draw batch
            //-----------------------------------------------------------------
            sf::RenderStates states;
            states.texture = &atlasTexture;
            states.shader = shader;

            window->draw(vertices, states);
        }

        return true;
    }

private:
    std::unordered_map<int, float> mParallaxFactors;
    std::unordered_map<std::string, std::unique_ptr<sf::Shader>> mShaders;
};

} // namespace ECSEngine
