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
        Rect localBounds;
        Rect currentBounds;
        Rect previousBounds;
        bool isStatic = true;

        CollisionComponent() = default;

        CollisionComponent(const Rect& local, bool isStatic)
                : localBounds(local),
                currentBounds(local),     // Will be converted to world space in CollisionSystem
                previousBounds(local),    // Will be replaced next frame
                isStatic(isStatic) {}
    };
}
