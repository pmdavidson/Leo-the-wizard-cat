#pragma once

#include <unordered_map>
#include <SFML/Graphics/Sprite.hpp>
#include <SFML/Graphics/Texture.hpp>
#include "MathUtil.h"
#include "SpriteManager.h"
#include <bitset>

namespace ECSEngine
{

    // The sprite rectangle is relative to the entity location in the world
    // This is only defined if an entity also has a LocationComponent.
    class SpriteComponent
    {
    public:
        SpriteID spriteId; // Sprite id associated with the entity
        Rect bounds;       // Bounds where the sprite is drawn (relative to entity location)
        bool inWorldSpace; // Whether the sprite is in world or screen space

        int layer = 0;

        // shader name (resolved in ShaderManager)
        std::string shaderName = "";

        SpriteComponent() = default;

        SpriteComponent(SpriteID id, const Rect &drawBounds, bool world = true, int layerNum, std::string shadeName)
            : spriteId(id),
              bounds(drawBounds),
              inWorldSpace(world),
              layer(layerNum),
              shaderName(shadeName) {}
    };
}
