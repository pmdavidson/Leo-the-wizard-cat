#include "Entity.h"

namespace ECSEngine
{

    Entity::Entity(EntityID id, const std::string& name)
        : mID(id), mName(name), mActive(true)
    {
    }

    EntityID Entity::GetID() const
    {
        return mID;
    }

    const std::string& Entity::GetName() const
    {
        return mName;
    }

    bool Entity::IsActive() const
    {
        return mActive;
    }

    void Entity::SetActive(bool active)
    {
        mActive = active;
    }

}
