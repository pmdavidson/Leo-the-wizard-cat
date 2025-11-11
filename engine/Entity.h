#pragma once

#include <array>
#include <tuple>

// An entity can have any component it is templated with,
// but does not have these installed by default.
// This is a base class for all entities in the ECS system.

namespace ECSEngine
{

    using EntityID = size_t;

    class Entity
    {
    };

}
