#pragma once

#include "SystemManager.h"
#include "Scene.h"
#include "SpawnComponent.h"
#include "LocationComponent.h"
#include "CollisionComponent.h"
#include "SpriteComponent.h"
#include "EnemyComponent.h"
#include "AnimationComponent.h"
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
							// Pick a random variant if available, otherwise use the spawner's default
							const SpawnComponent::SpawnVariant* variantPtr = nullptr;
							if (!spawnComp.variants.empty())
							{
								std::uniform_int_distribution<size_t> variantDist(0, spawnComp.variants.size() - 1);
								variantPtr = &spawnComp.variants[variantDist(gen)];
							}

							const auto& selectedAnimations = variantPtr ? variantPtr->animations : spawnComp.animations;
							const auto selectedSpriteId = variantPtr ? variantPtr->spriteId : spawnComp.spriteId;
							const auto& selectedDescription = variantPtr ? variantPtr->description : spawnComp.spawnDescription;

							auto &spawnerLocation = entityManager.template GetComponent<LocationComponent>(entityId);
							sf::Sprite sprite = spriteManager.GetSprite(selectedSpriteId);
							sf::IntRect textureRect = sprite.getTextureRect();

							Rect spriteBounds(0.0f, 0.0f,
											  static_cast<float>(textureRect.size.x),
											  static_cast<float>(textureRect.size.y));

							// Slimes are 96x32, collision box is narrower and shorter
							// Collision box: tighter fit around the slime
							float collisionWidth = 40.0f;
							float collisionHeight = 20.0f;
							float collisionOffsetX = (96.0f - collisionWidth) * 0.5f; // Center the collision box horizontally
							float collisionOffsetY = (32.0f - collisionHeight) * 0.5f; // Center the collision box vertically
							Rect collisionBounds = {collisionOffsetX, collisionOffsetY, collisionWidth, collisionHeight};

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

							float spriteOffsetY = collisionOffsetY + collisionHeight - 32.0f;
							SpriteComponent slimeSprite;
							slimeSprite.spriteId = selectedSpriteId;
							slimeSprite.bounds = Rect(0.0f, spriteOffsetY, spriteBounds.width, spriteBounds.height);
							slimeSprite.inWorldSpace = true;
							slimeSprite.layer = 2; // Enemies should be on layer 2 (just above world)
							entityManager.template AddComponent<SpriteComponent>(slimeId, slimeSprite);

							// Add EnemyComponent for slime
							EnemyComponent enemy;
							enemy.type = EnemyType::Slime;
							enemy.canMove = true; // Enable movement for slimes
							enemy.moveSpeed = 30.0f; // Slime movement speed
							// Initialize random direction and timer
							static std::mt19937 rng(std::random_device{}());
							std::uniform_real_distribution<float> dirDist(0.0f, 1.0f);
							enemy.moveDirection = (dirDist(rng) < 0.5f) ? -1.0f : 1.0f;
							std::uniform_real_distribution<float> timeDist(1.0f, 3.0f);
							enemy.directionChangeTimer = timeDist(rng);
							enemy.hp = 30.0f;
							enemy.maxHp = 30.0f;
							enemy.previousHp = 30.0f;
							enemy.contactDamage = 1.0f; // Slimes deal 1 damage per hit
							enemy.knockbackForce = 300.0f;
							enemy.isAlive = true;
							enemy.damageSoundName = "slime_damage";
							enemy.deathSoundName = "slime_die";

							// Default resistances = 1.0 (no resistance)
							for (auto &r : enemy.resistances)
								r = 1.0f;

							// Set elemental resistances based on slime type
							if (selectedDescription == "redSlime")
							{
								// Red: resist fire
								enemy.resistances[static_cast<size_t>(SpellType::Fire)] = 0.5f;
							}
							else if (selectedDescription == "blueSlime")
							{
								// Blue (water): resist water and fire
								enemy.resistances[static_cast<size_t>(SpellType::Water)] = 0.5f;
								enemy.resistances[static_cast<size_t>(SpellType::Fire)] = 0.5f;
							}
							else if (selectedDescription == "greenSlime")
							{
								// Green: resist water
								enemy.resistances[static_cast<size_t>(SpellType::Water)] = 0.5f;
							}
							else if (selectedDescription == "brownSlime")
							{
								// Brown/rock: resistant to all elements (set all to 0.5x), base HP stays at 30
								for (auto &r : enemy.resistances)
									r = 0.5f;
								enemy.hp = 50.0f;
								enemy.maxHp = 50.0f;
								enemy.previousHp = 50.0f;
							}

							entityManager.template AddComponent<EnemyComponent>(slimeId, enemy);

							// Add AnimationComponent for slime if animations are available
							if (!selectedAnimations.empty())
							{
								AnimationComponent slimeAnim;
								slimeAnim.animations = selectedAnimations;
								slimeAnim.frameDuration = 0.05f; // Faster animations
								
								// Start with idle animation if available
								if (slimeAnim.animations.count("idle") > 0)
								{
									slimeAnim.currentAnimation = "idle";
									slimeAnim.playing = true;
									slimeAnim.looping = true;
								}
								entityManager.template AddComponent<AnimationComponent>(slimeId, slimeAnim);
							}

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
