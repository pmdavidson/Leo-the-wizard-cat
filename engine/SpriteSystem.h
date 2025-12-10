#pragma once

#include "SystemManager.h"
#include "Scene.h"
#include "SpriteComponent.h"
#include "LocationComponent.h"
#include "CollisionComponent.h"
#include "HpComponent.h"
#include <cmath>
#include <SFML/Graphics/RectangleShape.hpp>

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
            std::string shaderName; // The key to the shaderName, current shader is shader name and vertex

            Rect dest;      // screen-space rect
            sf::IntRect texRect; // atlas rect
            bool flipX = false; // Flip horizontally
            
            EntityID entityId = 0; // Store entity ID for per-entity shader uniforms
            float flashIntensity = 0.0f; // Flash intensity for red flash shader
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
                // For UI elements (inWorldSpace = false), use LocationComponent position + bounds
                if (entityManager.template HasComponent<LocationComponent>(id))
                {
                    auto &loc = entityManager.template GetComponent<LocationComponent>(id);
                    screenRect = Rect(
                        loc.position + spriteComp.bounds.topLeft,
                        spriteComp.bounds.width,
                        spriteComp.bounds.height);
                }
                else
                {
                    screenRect = spriteComp.bounds;
                }
            }

            sf::Sprite sfSprite = spriteManager.GetSprite(spriteComp.spriteId);
            sf::IntRect texRect = sfSprite.getTextureRect();

            DrawItem item;
            item.layer = spriteComp.layer;
            item.parallax = GetParallaxFactor(spriteComp.layer);
            item.entityId = id;
            item.dest = screenRect;
            item.texRect = texRect;
            item.flipX = spriteComp.flipX;
            
            // Check if player is taking damage and apply red flash shader
            // Or check if rock shield is active and apply brown shader
            float flashIntensity = 0.0f;
            float shieldIntensity = 0.0f;
            if (entityManager.template HasComponent<HpComponent>(id))
            {
                auto &hpComp = entityManager.template GetComponent<HpComponent>(id);
                if (hpComp.damageFlashTimer > 0.0f)
                {
                    // Damage flash takes priority - apply red flash shader
                    float flashProgress = hpComp.damageFlashTimer / hpComp.damageFlashDuration;
                    flashIntensity = 1.0f - (1.0f - flashProgress) * (1.0f - flashProgress);
                    item.shaderName = "hurt";
                    item.flashIntensity = flashIntensity;
                }
                else if (hpComp.hasRockShield)
                {
                    // Rock shield is active - apply brown shader
                    item.shaderName = "rock_shield";
                    shieldIntensity = 1.0f; // Full shield intensity
                    item.flashIntensity = shieldIntensity; // Reuse flashIntensity field for shield
                }
                else
                {
                    item.shaderName = spriteComp.shaderName;
                }
            }
            else if (entityManager.template HasComponent<EnemyComponent>(id))
            {
                auto &enemyComp = entityManager.template GetComponent<EnemyComponent>(id);
                if (enemyComp.damageFlashTimer > 0.0f)
                {
                    float flashProgress = enemyComp.damageFlashTimer / enemyComp.damageFlashDuration;
                    flashIntensity = 1.0f - (1.0f - flashProgress) * (1.0f - flashProgress);
                    item.shaderName = "hurt";
                    item.flashIntensity = flashIntensity;
                }
                else
                {
                    item.shaderName = spriteComp.shaderName;
                }
            }
            else
            {
                item.shaderName = spriteComp.shaderName;
            }

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
            float batchFlashIntensity = drawItems[i].flashIntensity; // Store flash intensity for this batch

            sf::Shader *shader = nullptr;
            if (!currentShader.empty())
                shader = shaderManager.GetShader(currentShader);

            sf::VertexArray vertices(sf::PrimitiveType::Triangles);

            for (; i < drawItems.size() &&
                   drawItems[i].shaderName == currentShader;
                 ++i)
            {
                const DrawItem &item = drawItems[i];

                // World tiles (layers 0 and 1) should not have parallax applied
                // Parallax is only for background/foreground layers
                float x, y;
                if (item.layer == 0 || item.layer == 1)
                {
                    // World tiles: no parallax, just use screen position directly
                    x = std::round(item.dest.topLeft.x);
                    y = std::round(item.dest.topLeft.y);
                }
                else
                {
                    // Background/foreground layers: apply parallax
                    x = item.dest.topLeft.x * item.parallax;
                    y = item.dest.topLeft.y;
                }
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
            
            // Set flash intensity uniform if using hurt shader
            if (shader && currentShader == "hurt")
            {
                shader->setUniform("flashIntensity", batchFlashIntensity);
            }
            // Set shield intensity uniform if using rock_shield shader
            else if (shader && currentShader == "rock_shield")
            {
                shader->setUniform("shieldIntensity", batchFlashIntensity);
            }

            window->draw(vertices, states);
        }

        // Debug: draw collision bounding boxes (purple overlay)
        const bool debugDrawColliders = true;
        if (debugDrawColliders)
        {
            for (auto it = entityManager.begin(); it != entityManager.end(); ++it)
            {
                if (!it->isActive())
                    continue;
                EntityID id = it->getID();
                if (!entityManager.template HasComponent<CollisionComponent>(id))
                    continue;

                auto &col = entityManager.template GetComponent<CollisionComponent>(id);
                Rect worldRect = col.currentBounds;
                Rect screenRect = windowManager.WorldToWindow(worldRect);

                sf::RectangleShape box;
                box.setPosition(sf::Vector2f(
                    static_cast<float>(screenRect.topLeft.x),
                    static_cast<float>(screenRect.topLeft.y)));
                box.setSize(sf::Vector2f(
                    static_cast<float>(screenRect.width),
                    static_cast<float>(screenRect.height)));
                box.setFillColor(sf::Color(255, 0, 255, 80));      // purple 
                box.setOutlineThickness(2.0f);
                box.setOutlineColor(sf::Color(255, 0, 255, 220)); // bright purple outline
                window->draw(box);
            }
        }

        return true;
    }

private:
    std::unordered_map<int, float> mParallaxFactors;
};

}
