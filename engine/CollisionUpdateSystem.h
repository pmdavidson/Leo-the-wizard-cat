#pragma once

#include "SystemManager.h"
#include "Scene.h"
#include "CollisionComponent.h"
#include "LocationComponent.h"

namespace ECSEngine
{

	template <typename... Components>
	class CollisionUpdateSystem : public System<Components...>
	{
	public:
		bool Run(Scene<Components...> &scene) override
		{
			auto &entityManager = scene.GetEntityManager();

			for (auto it = entityManager.begin(); it != entityManager.end(); ++it)
			{
				if (!it->isActive())
					continue;

				EntityID id = it->getID();

				// Check if an entity has a collision comp and location comp
				if (!entityManager.template HasComponent<CollisionComponent>(id) ||
					!entityManager.template HasComponent<LocationComponent>(id))
					continue;

				auto &collisionComp = entityManager.template GetComponent<CollisionComponent>(id);
				auto &locationComp = entityManager.template GetComponent<LocationComponent>(id);

				// Calculate what the current world bounds should be
				Rect newCurrentBounds;
				newCurrentBounds.topLeft = locationComp.position + collisionComp.localBounds.topLeft;
				newCurrentBounds.width = collisionComp.localBounds.width;
				newCurrentBounds.height = collisionComp.localBounds.height;

				// Check if this is the first frame
				// If currentBounds equals localBounds, it hasn't been initialized yet
				if (collisionComp.currentBounds.topLeft.x == collisionComp.localBounds.topLeft.x &&
					collisionComp.currentBounds.topLeft.y == collisionComp.localBounds.topLeft.y &&
					collisionComp.currentBounds.width == collisionComp.localBounds.width)
				{
					// initialize both to the same world position
					collisionComp.previousBounds = newCurrentBounds;
					collisionComp.currentBounds = newCurrentBounds;
				}
				else
				{
					// store previous before updating current
					collisionComp.previousBounds = collisionComp.currentBounds;
					collisionComp.currentBounds = newCurrentBounds;
				}
			}

			return true;
		}
	};

} // namespace ECSEngine
