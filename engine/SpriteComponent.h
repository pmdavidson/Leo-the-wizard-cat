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
    class SpriteComponent {
        public:
            SpriteID spriteId;
            Rect bounds;
            bool inWorldSpace;

            SpriteComponent() = default;

            SpriteComponent(SpriteID id, const Rect& drawBounds, bool world = true)
                : spriteId(id),
                bounds(drawBounds),
                inWorldSpace(world) {}
        };
}
