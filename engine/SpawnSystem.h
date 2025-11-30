#pragma once

#include "SystemManager.h"
#include "Scene.h"
#include "SpawnComponent.h"
#include "LocationComponent.h"
#include "CollisionComponent.h"
#include "SpriteComponent.h"
#include <cmath>
#include <random>

namespace ECSEngine
{

	template <typename... Components>
	class SpawnSystem : public System<Components...>
	{
	public:
		bool Run(Scene<Components...> &scene) override
		{
			auto &entityManager = scene.GetEntityManager();
			auto &spriteManager = scene.GetSpriteManager();
			float deltaTime = 1.0f / 60.0f;

			static std::random_device rd;
			static std::mt19937 gen(rd());
			static std::uniform_real_distribution<float> angleDist(0.0f, 2.0f * 3.14159f);
			static std::uniform_real_distribution<float> speedDist(50.0f, 150.0f);

			for (auto it = entityManager.begin(); it != entityManager.end(); ++it)
			{
				if (!it->isActive())
					continue;
				EntityID entityId = it->getID();

				if (entityManager.template HasComponent<SpawnComponent>(entityId))
				{
					auto &spawnComp = entityManager.template GetComponent<SpawnComponent>(entityId);

					spawnComp.timeToNextSpawn -= deltaTime;

					if (spawnComp.timeToNextSpawn <= 0.0f && spawnComp.totalSpawnEvents > 0)
					{
						if (entityManager.template HasComponent<LocationComponent>(entityId))
						{
							auto &spawnerLocation = entityManager.template GetComponent<LocationComponent>(entityId);
							sf::Sprite &sprite = spriteManager.GetSprite(spawnComp.spriteId);
							sf::IntRect textureRect = sprite.getTextureRect();

							Rect spriteBounds(0.0f, 0.0f,
											  static_cast<float>(textureRect.size.x),
											  static_cast<float>(textureRect.size.y));

							Rect collisionBounds = {0.0f, 0.0f, spawnComp.tileW, spawnComp.tileH};

							EntityID starId = entityManager.CreateEntity("star");

							entityManager.template AddComponent<LocationComponent>(starId, LocationComponent(spawnerLocation.position));

							float angle = angleDist(gen);
							float speed = speedDist(gen);
							MovementComponent starMovement;
							starMovement.velocity = Point2D(std::cos(angle) * speed, std::sin(angle) * speed);
							entityManager.template AddComponent<MovementComponent>(starId, starMovement);

							entityManager.template AddComponent<GravityComponent>(starId, GravityComponent(Point2D(0, 600.0f)));

							CollisionComponent starCollision(collisionBounds, false);
							starCollision.currentBounds = collisionBounds;
							starCollision.previousBounds = collisionBounds;
							entityManager.template AddComponent<CollisionComponent>(starId, starCollision);

							SpriteComponent starSprite;
							starSprite.spriteId = spawnComp.spriteId;
							starSprite.bounds = spriteBounds;
							starSprite.inWorldSpace = true;
							entityManager.template AddComponent<SpriteComponent>(starId, starSprite);

							spawnComp.timeToNextSpawn = spawnComp.timeBetweenSpawns;
							spawnComp.totalSpawnEvents--;
						}
					}
				}
			}

			return true;
		}
	};

} // namespace ECSEngine
