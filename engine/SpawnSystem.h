#pragma once

#include "SystemManager.h"
#include "Scene.h"
#include "SpawnComponent.h"
#include "LocationComponent.h"
#include "CollisionComponent.h"
#include "SpriteComponent.h"
#include "EnemyComponent.h"
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

							// Slimes are 96x32, collision box matches sprite
							Rect collisionBounds = {0.0f, 0.0f, spawnComp.tileW, spawnComp.tileH};

							EntityID slimeId = entityManager.CreateEntity("slime");

							entityManager.template AddComponent<LocationComponent>(slimeId, LocationComponent(spawnerLocation.position));

							// Slimes don't move for now, but have gravity
							MovementComponent slimeMovement;
							slimeMovement.velocity = Point2D(0.0f, 0.0f);
							entityManager.template AddComponent<MovementComponent>(slimeId, slimeMovement);

							// Add gravity so slimes fall
							entityManager.template AddComponent<GravityComponent>(slimeId, GravityComponent(Point2D(0, 600.0f)));

							CollisionComponent slimeCollision(collisionBounds, false);
							slimeCollision.currentBounds = collisionBounds;
							slimeCollision.previousBounds = collisionBounds;
							entityManager.template AddComponent<CollisionComponent>(slimeId, slimeCollision);

							SpriteComponent slimeSprite;
							slimeSprite.spriteId = spawnComp.spriteId;
							slimeSprite.bounds = spriteBounds;
							slimeSprite.inWorldSpace = true;
							entityManager.template AddComponent<SpriteComponent>(slimeId, slimeSprite);

							// Add EnemyComponent for slime
							EnemyComponent enemy;
							enemy.type = EnemyType::Slime;
							enemy.hp = 10.0f;
							enemy.maxHp = 10.0f;
							enemy.contactDamage = 10.0f;
							enemy.knockbackForce = 300.0f;
							enemy.isAlive = true;
							enemy.canMove = false;
							enemy.damageSoundName = "slime_damage_1";
							enemy.deathSoundName = "slime_die";
							entityManager.template AddComponent<EnemyComponent>(slimeId, enemy);

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
