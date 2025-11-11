#pragma once

#include <string>
#include <cstddef>

// An entity can have any component it is templated with,
// but does not have these installed by default.
// This is a base class for all entities in the ECS system.

namespace ECSEngine {
    
    using EntityID = size_t;

    class Entity {
    public:
        Entity(EntityID id, const std::string& name);

        [[nodiscard]] EntityID GetID() const;
        [[nodiscard]] const std::string& GetName() const;
        [[nodiscard]] bool IsActive() const;

        void SetActive(bool active);

    private:
        EntityID mID;
        std::string mName;
        bool mActive;
    };
}
