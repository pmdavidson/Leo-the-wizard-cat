#pragma once

#include <unordered_map>
#include <SFML/Graphics/Sprite.hpp>
#include <SFML/Graphics/Texture.hpp>
#include "MathUtil.h"
#include <bitset>

namespace ECSEngine
{

    struct CollisionFlags {
        bool top = false;
        bool bottom = false;
        bool left = false;
        bool right = false;
    };

    struct CollisionComponent {
        CollisionFlags collidedSides;
        Rect currentBounds;
        Rect previousBounds;
        bool isStatic = false;

        CollisionComponent() = default;

        CollisionComponent(const Rect& bounds, bool isStatic = false)
            : currentBounds(bounds),
            previousBounds(bounds),
            isStatic(isStatic) {}
    };

}
