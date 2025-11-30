#pragma once

#include "SystemManager.h"
#include "Scene.h"
#include "LocationComponent.h"
#include "InputComponent.h"
#include "CollisionComponent.h"

namespace ECSEngine
{

	template <typename... Components>
	class GravitySystem : public System<Components...>
	{
	public:
		bool Run(Scene<Components...> &scene) override
		{
			auto &entityManager = scene.GetEntityManager();
			float deltaTime = 1.0f / 60.0f; // assuming 60FPS?

			for (auto it = entityManager.begin(); it != entityManager.end(); ++it)
			{
				if (!it->isActive())
					continue;
				EntityID entityId = it->getID();

				if (entityManager.template HasComponent<GravityComponent>(entityId) &&
					entityManager.template HasComponent<MovementComponent>(entityId))
				{
					auto &gravityComp = entityManager.template GetComponent<GravityComponent>(entityId);
					auto &movementComp = entityManager.template GetComponent<MovementComponent>(entityId);

					// Check if player is on wall while falling
					bool onWallWhileFalling = false;
					if (entityManager.template HasComponent<InputComponent>(entityId) &&
						entityManager.template HasComponent<CollisionComponent>(entityId))
					{
						auto &collisionComp = entityManager.template GetComponent<CollisionComponent>(entityId);
						bool onWall = collisionComp.collidedSides.left || collisionComp.collidedSides.right;
						bool falling = movementComp.velocity.y > 0;
						onWallWhileFalling = onWall && falling;
					}

					// Apply gravity, reduced if sliding on wall
					float gravityMultiplier = onWallWhileFalling ? 0.3f : 1.0f;
					movementComp.velocity += gravityComp.acceleration * deltaTime * gravityMultiplier;
				}
			}

			return true;
		}
	};

} // namespace ECSEngine
