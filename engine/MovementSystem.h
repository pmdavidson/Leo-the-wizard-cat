#pragma once

#include "SystemManager.h"
#include "Scene.h"
#include "LocationComponent.h"
#include "SpriteComponent.h"
#include "InputComponent.h"

namespace ECSEngine
{

	template <typename... Components>
	class MovementSystem : public System<Components...>
	{
	public:
		bool Run(Scene<Components...> &scene) override
		{
			auto &entityManager = scene.GetEntityManager();
			float deltaTime = 1.0f / 60.0f;

			// Update locations based on velocity
			for (auto it = entityManager.begin(); it != entityManager.end(); ++it)
			{
				if (!it->isActive())
					continue;
				EntityID entityId = it->getID();

				if (entityManager.template HasComponent<LocationComponent>(entityId) &&
					entityManager.template HasComponent<MovementComponent>(entityId))
				{
					auto &locationComp = entityManager.template GetComponent<LocationComponent>(entityId);
					auto &movementComp = entityManager.template GetComponent<MovementComponent>(entityId);

					// Update position based on velocity
					locationComp.position.x += movementComp.velocity.x * deltaTime;
					locationComp.position.y += movementComp.velocity.y * deltaTime;
					
					// Update sprite flip based on velocity for entities without InputComponent
					// (InputComponent entities are handled in InputSystem)
					if (!entityManager.template HasComponent<InputComponent>(entityId) &&
						entityManager.template HasComponent<SpriteComponent>(entityId))
					{
						auto &spriteComp = entityManager.template GetComponent<SpriteComponent>(entityId);
						// Flip based on horizontal velocity direction
						if (std::abs(movementComp.velocity.x) > 5.0f)
						{
							spriteComp.flipX = (movementComp.velocity.x < 0);
						}
					}
				}
			}

			return true;
		}
	};

} // namespace ECSEngine
