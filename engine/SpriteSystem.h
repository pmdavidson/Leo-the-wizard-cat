#pragma once

#include "SystemManager.h"
#include "Scene.h"
#include "SpriteComponent.h"
#include "LocationComponent.h"

namespace ECSEngine
{

template <typename... Components>
class SpriteSystem : public System<Components...>
{
public:
    SpriteSystem() = default;
    ~SpriteSystem() = default;

    // Parallax control
    void SetParallaxFactor(int layer, float factor)
    {
        mParallaxFactors[layer] = factor;
    }

    float GetParallaxFactor(int layer) const
    {
        auto it = mParallaxFactors.find(layer);
        return (it != mParallaxFactors.end()) ? it->second : 1.0f;
    }

    bool Run(Scene<Components...> &scene) override
    {
        auto &entityManager = scene.GetEntityManager();
        auto &spriteManager = scene.GetSpriteManager();
        auto &windowManager = scene.GetWindowManager();
        auto &shaderManager = scene.GetShaderManager();

        sf::RenderWindow *window = windowManager.GetWindow();
        if (!window)
            return false;

        sf::Texture &atlasTexture = spriteManager.GetTexture();

        struct DrawItem
        {
            int layer = 0;
            float parallax = 1.0f;
            std::string shaderName;

            Rect dest;      // screen-space rect
            sf::IntRect texRect; // atlas rect
            bool flipX = false; // Flip horizontally
        };

        std::vector<DrawItem> drawItems;
        drawItems.reserve(256);

        // Collect draw items
        for (auto it = entityManager.begin(); it != entityManager.end(); ++it)
        {
            if (!it->isActive())
                continue;

            EntityID id = it->getID();
            if (!entityManager.template HasComponent<SpriteComponent>(id))
                continue;

            auto &spriteComp = entityManager.template GetComponent<SpriteComponent>(id);

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

            sf::Sprite sfSprite = spriteManager.GetSprite(spriteComp.spriteId);
            sf::IntRect texRect = sfSprite.getTextureRect();

            DrawItem item;
            item.layer = spriteComp.layer;
            item.parallax = GetParallaxFactor(spriteComp.layer);
            item.shaderName = spriteComp.shaderName;
            item.dest = screenRect;
            item.texRect = texRect;
            item.flipX = spriteComp.flipX;

            drawItems.push_back(item);
        }

        if (drawItems.empty())
            return true;

        // Sort by layer, then shader
        std::sort(drawItems.begin(), drawItems.end(),
                  [](const DrawItem &a, const DrawItem &b)
                  {
                      if (a.layer == b.layer)
                          return a.shaderName < b.shaderName;
                      return a.layer < b.layer;
                  });

        // Batch render
        std::size_t i = 0;
        while (i < drawItems.size())
        {
            const std::string &currentShader = drawItems[i].shaderName;

            sf::Shader *shader = nullptr;
            if (!currentShader.empty())
                shader = shaderManager.GetShader(currentShader);

            sf::VertexArray vertices(sf::PrimitiveType::Triangles);

            for (; i < drawItems.size() &&
                   drawItems[i].shaderName == currentShader;
                 ++i)
            {
                const DrawItem &item = drawItems[i];

                float x = item.dest.topLeft.x * item.parallax;
                float y = item.dest.topLeft.y;
                float w = item.dest.width;
                float h = item.dest.height;

                float u0 = item.texRect.position.x;
                float v0 = item.texRect.position.y;
                float u1 = u0 + item.texRect.size.x;
                float v1 = v0 + item.texRect.size.y;

                // Flip UV coordinates horizontally if flipX is true
                if (item.flipX)
                {
                    std::swap(u0, u1);
                }

                vertices.append(sf::Vertex({x,     y},     sf::Color::White, {u0, v0}));
                vertices.append(sf::Vertex({x + w, y},     sf::Color::White, {u1, v0}));
                vertices.append(sf::Vertex({x + w, y + h}, sf::Color::White, {u1, v1}));

                vertices.append(sf::Vertex({x,     y},     sf::Color::White, {u0, v0}));
                vertices.append(sf::Vertex({x + w, y + h}, sf::Color::White, {u1, v1}));
                vertices.append(sf::Vertex({x,     y + h}, sf::Color::White, {u0, v1}));
            }

            sf::RenderStates states;
            states.texture = &atlasTexture; //atlas texture is the entire texture
            states.shader = shader;

            window->draw(vertices, states);
        }

        return true;
    }

private:
    std::unordered_map<int, float> mParallaxFactors;
};

}
