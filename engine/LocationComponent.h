#pragma once

#include <unordered_map>
#include "MathUtil.h"
#include <bitset>

namespace ECSEngine
{

    struct LocationComponent
    {
        Point2D position;

        LocationComponent() : position{0.0f, 0.0f} {}
        explicit LocationComponent(const Point2D &pos) : position(pos) {}
    };

    struct MovementComponent
    {
        Point2D velocity;

        MovementComponent() : velocity{5.0f, 5.0f} {}
        explicit MovementComponent(const Point2D &vel) : velocity(vel) {}
    };

    struct GravityComponent
    {
        Point2D acceleration;

        GravityComponent() : acceleration{1.0f, 1.0f} {}
        explicit GravityComponent(const Point2D &acc) : acceleration(acc) {}
    };

    struct JumpComponent {
        bool hasDoubleJump = false;
        int jumpsUsed = 0;
        int maxJumps = 1; // default
    };

}
