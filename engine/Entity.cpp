#include "Entity.h"

namespace ECSEngine
{

    Entity::Entity(EntityID id, const std::string& name)
        : mID(id), mName(name), mActive(true)
    {
    }

    EntityID Entity::getID() const
    {
        return mID;
    }

    const std::string& Entity::getName() const
    {
        return mName;
    }

    bool Entity::isActive() const
    {
        return mActive;
    }

    void Entity::setActive(bool active)
    {
        mActive = active;
    }

}
