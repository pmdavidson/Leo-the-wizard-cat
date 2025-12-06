#pragma once

#include "SystemManager.h"
#include "Scene.h"
#include "LocationComponent.h"

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
				}
			}

			return true;
		}
	};

} // namespace ECSEngine
