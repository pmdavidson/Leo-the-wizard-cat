#pragma once

#include "SystemManager.h"
#include "Scene.h"
#include "CollisionComponent.h"
#include "LocationComponent.h"
#include "InputComponent.h"
#include "SpawnComponent.h"
#include "ScoreComponent.h"
#include "CameraComponent.h"
#include "SoundManager.h"
#include "ECSEngine.h"
#include "EnemyComponent.h"
#include "HpComponent.h"
#include "ProjectileComponent.h"
#include "CheckpointComponent.h"
#include <cmath>
#include <random>

namespace ECSEngine
{

	template <typename... Components>
	class CollisionSystem : public System<Components...>
	{
		public:
		bool Run(Scene<Components...> &scene) override
		{
			auto &entityManager = scene.GetEntityManager();
			auto &soundManager = scene.GetSoundManager();

			// Update all current bounds after movement and clear collision flags
			for (auto it = entityManager.begin(); it != entityManager.end(); ++it)
			{
				if (!it->isActive())
					continue;
				EntityID id = it->getID();

				if (!entityManager.template HasComponent<CollisionComponent>(id) ||
					!entityManager.template HasComponent<LocationComponent>(id))
					continue;

				auto &collisionComp = entityManager.template GetComponent<CollisionComponent>(id);
				auto &locationComp = entityManager.template GetComponent<LocationComponent>(id);

				collisionComp.currentBounds.topLeft = locationComp.position + collisionComp.localBounds.topLeft;
				collisionComp.currentBounds.width = collisionComp.localBounds.width;
				collisionComp.currentBounds.height = collisionComp.localBounds.height;

				collisionComp.collidedSides = {};
			}

			// Check and resolve collisions
			for (auto itA = entityManager.begin(); itA != entityManager.end(); ++itA)
			{
				auto &entityA = *itA;
				if (!entityA.isActive())
					continue;
				EntityID idA = entityA.getID();

				if (!entityManager.template HasComponent<CollisionComponent>(idA) ||
					!entityManager.template HasComponent<LocationComponent>(idA))
					continue;

				auto &colA = entityManager.template GetComponent<CollisionComponent>(idA);
				auto &LocA = entityManager.template GetComponent<LocationComponent>(idA);

				for (auto itB = std::next(itA); itB != entityManager.end(); ++itB)
				{
					auto &entityB = *itB;
					if (!entityB.isActive())
						continue;
					EntityID idB = entityB.getID();

					if (!entityManager.template HasComponent<CollisionComponent>(idB) ||
						!entityManager.template HasComponent<LocationComponent>(idB))
						continue;

					auto &colB = entityManager.template GetComponent<CollisionComponent>(idB);
					auto &LocB = entityManager.template GetComponent<LocationComponent>(idB);

					if (colA.isStatic && colB.isStatic)
						continue;

					Rect BoundsA(colA.currentBounds.topLeft, colA.currentBounds.width, colA.currentBounds.height);
					Rect BoundsB(colB.currentBounds.topLeft, colB.currentBounds.width, colB.currentBounds.height);

					if (!BoundsA.intersects(BoundsB))
						continue;

					// Identify entity types
					bool isPlayerA = entityManager.template HasComponent<InputComponent>(idA);
					bool isPlayerB = entityManager.template HasComponent<InputComponent>(idB);
					bool isSpawnerA = entityManager.template HasComponent<SpawnComponent>(idA);
					bool isSpawnerB = entityManager.template HasComponent<SpawnComponent>(idB);
					bool isEnemyA = entityManager.template HasComponent<EnemyComponent>(idA);
					bool isEnemyB = entityManager.template HasComponent<EnemyComponent>(idB);
					bool isProjectileA = entityManager.template HasComponent<ProjectileComponent>(idA);
					bool isProjectileB = entityManager.template HasComponent<ProjectileComponent>(idB);
					bool isCampfireA = entityManager.template HasComponent<CampfireComponent>(idA);
					bool isCampfireB = entityManager.template HasComponent<CampfireComponent>(idB);

					// Skip collision resolution for player-campfire (player walks through campfires)
					// But still process projectile-campfire later
					bool isPlayerCampfireCollision = (isPlayerA && isCampfireB) || (isPlayerB && isCampfireA);

					bool isStarA = !isPlayerA && !isSpawnerA && !isEnemyA && !isProjectileA && !isCampfireA &&
								   entityManager.template HasComponent<CollisionComponent>(idA) &&
								   !entityManager.template GetComponent<CollisionComponent>(idA).isStatic &&
								   (entityManager.template HasComponent<GravityComponent>(idA) ||
									!entityManager.template HasComponent<MovementComponent>(idA));
					bool isStarB = !isPlayerB && !isSpawnerB && !isEnemyB && !isProjectileB && !isCampfireB &&
								   entityManager.template HasComponent<CollisionComponent>(idB) &&
								   !entityManager.template GetComponent<CollisionComponent>(idB).isStatic &&
								   (entityManager.template HasComponent<GravityComponent>(idB) ||
									!entityManager.template HasComponent<MovementComponent>(idB));

					// Skip collision resolution entirely if a projectile overlaps its owner (avoid pushback/stick)
					if ((isProjectileA && isPlayerB &&
						 entityManager.template GetComponent<ProjectileComponent>(idA).ownerEntityId == idB) ||
						(isProjectileB && isPlayerA &&
						 entityManager.template GetComponent<ProjectileComponent>(idB).ownerEntityId == idA))
					{
						continue;
					}

					Point2D preCollisionVelocityA(0.0f, 0.0f);
					Point2D preCollisionVelocityB(0.0f, 0.0f);

					if (isPlayerA && entityManager.template HasComponent<MovementComponent>(idA))
						preCollisionVelocityA = entityManager.template GetComponent<MovementComponent>(idA).velocity;
					if (isPlayerB && entityManager.template HasComponent<MovementComponent>(idB))
						preCollisionVelocityB = entityManager.template GetComponent<MovementComponent>(idB).velocity;

					// Skip collision resolution for player-campfire (player walks through)
					// Projectile-campfire is handled separately below
					if (isPlayerCampfireCollision)
					{
						// Don't resolve collision, but still check special interactions below
					}
					// Resolve collision
					else if (colB.isStatic)
					{
						ResolveAABBCollision(
							BoundsA, BoundsB,
							colA.previousBounds, colB.previousBounds,
							colA.collidedSides, colB.collidedSides, preCollisionVelocityA);

						LocA.position = BoundsA.topLeft - colA.localBounds.topLeft;
						colA.currentBounds = BoundsA;

						// Zero velocity for any entity with MovementComponent when colliding with static geometry
						if (entityManager.template HasComponent<MovementComponent>(idA))
						{
							auto &vel = entityManager.template GetComponent<MovementComponent>(idA);
							if (colA.collidedSides.bottom || colA.collidedSides.top)
								vel.velocity.y = 0;
							if (colA.collidedSides.left || colA.collidedSides.right)
								vel.velocity.x = 0;
						}
					}
					else if (colA.isStatic)
					{
						ResolveAABBCollision(
							BoundsB, BoundsA,
							colB.previousBounds, colA.previousBounds,
							colB.collidedSides, colA.collidedSides, preCollisionVelocityB);

						LocB.position = BoundsB.topLeft - colB.localBounds.topLeft;
						colB.currentBounds = BoundsB;

						// Zero velocity for any entity with MovementComponent when colliding with static geometry
						if (entityManager.template HasComponent<MovementComponent>(idB))
						{
							auto &vel = entityManager.template GetComponent<MovementComponent>(idB);
							if (colB.collidedSides.bottom || colB.collidedSides.top)
								vel.velocity.y = 0;
							if (colB.collidedSides.left || colB.collidedSides.right)
								vel.velocity.x = 0;
						}
					}
					else
					{
						ResolveAABBCollision(
							BoundsA, BoundsB,
							colA.previousBounds, colB.previousBounds,
							colA.collidedSides, colB.collidedSides, preCollisionVelocityA);

						LocA.position = BoundsA.topLeft - colA.localBounds.topLeft;
						colA.currentBounds = BoundsA;

						ResolveAABBCollision(
							BoundsB, BoundsA,
							colB.previousBounds, colA.previousBounds,
							colB.collidedSides, colA.collidedSides, preCollisionVelocityB);

						LocB.position = BoundsB.topLeft - colB.localBounds.topLeft;
						colB.currentBounds = BoundsB;
					}

					// Trigger camera shake
					if (isPlayerA && colB.isStatic)
					{
						playerCollisionCheck(scene, colA.collidedSides, preCollisionVelocityA);
					}
					else if (isPlayerB && colA.isStatic)
					{
						playerCollisionCheck(scene, colB.collidedSides, preCollisionVelocityB);
					}

					bool removedA = false;
					bool removedB = false;

					// Handle player-star collisions
					if ((isPlayerA && isStarB) || (isPlayerB && isStarA))
					{
						soundManager.PlaySound("star_collect");

						for (auto scoreIt = entityManager.begin(); scoreIt != entityManager.end(); ++scoreIt)
						{
							EntityID scoreEntityId = scoreIt->getID();
							if (entityManager.template HasComponent<ScoreComponent>(scoreEntityId))
							{
								entityManager.template GetComponent<ScoreComponent>(scoreEntityId).currentScore += 10;
								break;
							}
						}

						if (isStarA)
						{
							entityManager.RemoveEntity(idA);
							removedA = true;
						}
						else
						{
							entityManager.RemoveEntity(idB);
							removedB = true;
						}
					}

					// Handle player-enemy collisions
					if ((isPlayerA && isEnemyB) || (isPlayerB && isEnemyA))
					{
						EntityID playerId = isPlayerA ? idA : idB;
						EntityID enemyId = isEnemyA ? idA : idB;

						auto &enemy = entityManager.template GetComponent<EnemyComponent>(enemyId);

						// Check if player has HP component and isn't invincible
						bool playerCanTakeDamage = false;
						if (entityManager.template HasComponent<HpComponent>(playerId))
						{
							auto &playerHp = entityManager.template GetComponent<HpComponent>(playerId);
							playerCanTakeDamage = playerHp.isAlive && playerHp.invincibilityTimer <= 0.0f;
						}

						// Only damage if enemy is alive and player can take damage
						if (enemy.isAlive && playerCanTakeDamage)
						{
							// Deal damage to player through HpComponent
							if (entityManager.template HasComponent<HpComponent>(playerId))
							{
								auto &playerHp = entityManager.template GetComponent<HpComponent>(playerId);
								playerHp.currentHp -= static_cast<int>(enemy.contactDamage);
								playerHp.invincibilityTimer = playerHp.invincibilityDuration;
							}

							// Apply knockback to player
							if (entityManager.template HasComponent<MovementComponent>(playerId))
							{
								auto &playerMovement = entityManager.template GetComponent<MovementComponent>(playerId);
								auto &playerLoc = entityManager.template GetComponent<LocationComponent>(playerId);
								auto &enemyLoc = entityManager.template GetComponent<LocationComponent>(enemyId);

								// Knockback direction: away from enemy
								float knockbackDirX = (playerLoc.position.x > enemyLoc.position.x) ? 1.0f : -1.0f;

								// Apply knockback velocity
								playerMovement.velocity.x = knockbackDirX * enemy.knockbackForce;
								playerMovement.velocity.y = -enemy.knockbackForce * 0.5f; // Slight upward knockback
							}

							// Play player hurt sound
							soundManager.PlaySound("take_damage");
						}
					}

					// Handle projectile-enemy collisions (player projectiles hitting enemies)
					if ((isProjectileA && isEnemyB) || (isProjectileB && isEnemyA))
					{
						EntityID projectileId = isProjectileA ? idA : idB;
						EntityID enemyId = isEnemyA ? idA : idB;

						auto &projectile = entityManager.template GetComponent<ProjectileComponent>(projectileId);
						auto &enemy = entityManager.template GetComponent<EnemyComponent>(enemyId);

						// Skip if projectile was fired by this enemy (no self-damage)
						// Also skip if projectile was fired by any enemy (no friendly fire between enemies)
						bool isEnemyProjectile = entityManager.template HasComponent<EnemyComponent>(projectile.ownerEntityId);

						// Only damage if projectile is active, enemy is alive, and it's not an enemy's projectile
						if (projectile.active && enemy.isAlive && !isEnemyProjectile)
						{
							// Apply elemental resistance if present
							float dmgMultiplier = 1.0f;
							size_t spellIdx = static_cast<size_t>(projectile.spellType);
							if (spellIdx < enemy.resistances.size() && enemy.resistances[spellIdx] > 0.0f)
							{
								dmgMultiplier = enemy.resistances[spellIdx];
							}

							float appliedDamage = projectile.damage * dmgMultiplier;
							enemy.hp -= appliedDamage;

							// Play enemy damage sound 
							static std::mt19937 rng(std::random_device{}());
							std::uniform_int_distribution<int> dist(1, 2);
							std::string damageSoundName = enemy.damageSoundName + "_" + std::to_string(dist(rng));
							soundManager.PlaySound(damageSoundName);

							// Remove the projectile
							projectile.active = false;

							// Add score for hitting an enemy
							for (auto scoreIt = entityManager.begin(); scoreIt != entityManager.end(); ++scoreIt)
							{
								EntityID scoreEntityId = scoreIt->getID();
								if (entityManager.template HasComponent<ScoreComponent>(scoreEntityId))
								{
									entityManager.template GetComponent<ScoreComponent>(scoreEntityId).currentScore += 5;
									break;
								}
							}
						}
					}

					// Handle projectile-player collisions (enemy projectiles hitting player)
					// Skip collision response if the projectile belongs to this player (no self-collide)
					bool isPlayerProjectileHitsOwner =
						(isProjectileA && isPlayerB &&
						 entityManager.template GetComponent<ProjectileComponent>(idA).ownerEntityId == idB) ||
						(isProjectileB && isPlayerA &&
						 entityManager.template GetComponent<ProjectileComponent>(idB).ownerEntityId == idA);

					if (!isPlayerProjectileHitsOwner &&
						((isProjectileA && isPlayerB) || (isProjectileB && isPlayerA)))
					{
						EntityID projectileId = isProjectileA ? idA : idB;
						EntityID playerId = isPlayerA ? idA : idB;

						auto &projectile = entityManager.template GetComponent<ProjectileComponent>(projectileId);

						// Only process enemy projectiles (skip player's own projectiles)
						bool isEnemyProjectile = entityManager.template HasComponent<EnemyComponent>(projectile.ownerEntityId);

						// Check if player can take damage
						bool playerCanTakeDamage = false;
						if (entityManager.template HasComponent<HpComponent>(playerId))
						{
							auto &playerHp = entityManager.template GetComponent<HpComponent>(playerId);
							playerCanTakeDamage = playerHp.isAlive && playerHp.invincibilityTimer <= 0.0f;
						}

						// Only damage if projectile is active, it's an enemy projectile, and player can take damage
						if (projectile.active && isEnemyProjectile && playerCanTakeDamage)
						{
							// Deal damage to player
							if (entityManager.template HasComponent<HpComponent>(playerId))
							{
								auto &playerHp = entityManager.template GetComponent<HpComponent>(playerId);
								playerHp.currentHp -= static_cast<int>(projectile.damage);
								playerHp.invincibilityTimer = playerHp.invincibilityDuration;
							}

							// Play player hurt sound
							soundManager.PlaySound("take_damage");

							// Deactivate the projectile
							projectile.active = false;
						}
					}

					// Handle projectile-campfire collisions (player fire spells lights campfires)
					if ((isProjectileA && isCampfireB) || (isProjectileB && isCampfireA))
					{
						EntityID projectileId = isProjectileA ? idA : idB;
						EntityID campfireId = isCampfireA ? idA : idB;

						auto &projectile = entityManager.template GetComponent<ProjectileComponent>(projectileId);
						auto &campfire = entityManager.template GetComponent<CampfireComponent>(campfireId);

						// Only player fire projectiles can light campfires
						bool isPlayerProjectile = entityManager.template HasComponent<InputComponent>(projectile.ownerEntityId);
						bool isFireSpell = projectile.spellType == SpellType::Fire;

						if (projectile.active && !campfire.isLit && isPlayerProjectile && isFireSpell)
						{
							// Light the campfire
							campfire.isLit = true;

							// Update campfire sprite to lit version
							if (entityManager.template HasComponent<SpriteComponent>(campfireId))
							{
								auto &sprite = entityManager.template GetComponent<SpriteComponent>(campfireId);
								sprite.spriteId = campfire.litSpriteId;
							}

							// Get campfire position for checkpoint
							auto &campLoc = entityManager.template GetComponent<LocationComponent>(campfireId);
							Point2D checkpointPos = campLoc.position;

							// Activate checkpoint for the player
							for (auto playerIt = entityManager.begin(); playerIt != entityManager.end(); ++playerIt)
							{
								if (!playerIt->isActive())
									continue;
								EntityID playerId = playerIt->getID();

								if (!entityManager.template HasComponent<CheckpointComponent>(playerId))
									continue;

								auto &checkpoint = entityManager.template GetComponent<CheckpointComponent>(playerId);
								checkpoint.ActivateCheckpoint(campfire.checkpointIndex, checkpointPos);
							}

							// Deactivate the projectile
							projectile.active = false;
						}
					}

					// Handle star collisions
					if (!removedA && isStarA && entityManager.template HasComponent<MovementComponent>(idA))
					{
						auto &movementCompA = entityManager.template GetComponent<MovementComponent>(idA);

						if (colB.isStatic)
						{
							const float speedLoss = 0.5f;
							if (colA.collidedSides.left || colA.collidedSides.right)
								movementCompA.velocity.x = -movementCompA.velocity.x * speedLoss;
							if (colA.collidedSides.top || colA.collidedSides.bottom)
							{
								movementCompA.velocity.y = -movementCompA.velocity.y * speedLoss;
							}
						}
						else if (!removedB && isStarB)
						{
							bool isVerticalCollision = colA.collidedSides.top || colA.collidedSides.bottom ||
													   colB.collidedSides.top || colB.collidedSides.bottom;

							if (isVerticalCollision)
							{
								if (entityManager.template HasComponent<GravityComponent>(idA))
									entityManager.template RemoveComponent<GravityComponent>(idA);
								if (entityManager.template HasComponent<MovementComponent>(idA))
									entityManager.template RemoveComponent<MovementComponent>(idA);

								if (entityManager.template HasComponent<GravityComponent>(idB))
									entityManager.template RemoveComponent<GravityComponent>(idB);
								if (entityManager.template HasComponent<MovementComponent>(idB))
									entityManager.template RemoveComponent<MovementComponent>(idB);
							}
							else
							{
								movementCompA.velocity = Point2D(0.0f, 0.0f);
								if (entityManager.template HasComponent<MovementComponent>(idB))
									entityManager.template GetComponent<MovementComponent>(idB).velocity = Point2D(0.0f, 0.0f);
							}
						}
					}
					else if (!removedB && isStarB && entityManager.template HasComponent<MovementComponent>(idB))
					{
						auto &movementCompB = entityManager.template GetComponent<MovementComponent>(idB);

						if (colA.isStatic)
						{
							const float speedLoss = 0.75f;
							if (colB.collidedSides.left || colB.collidedSides.right)
								movementCompB.velocity.x = -movementCompB.velocity.x * speedLoss;
							if (colB.collidedSides.top || colB.collidedSides.bottom)
							{
								movementCompB.velocity.y = -movementCompB.velocity.y * speedLoss;
							}
						}
						else if (!removedA && isStarA)
						{
							bool isVerticalCollision = colA.collidedSides.top || colA.collidedSides.bottom ||
													   colB.collidedSides.top || colB.collidedSides.bottom;

							if (isVerticalCollision)
							{
								if (entityManager.template HasComponent<GravityComponent>(idA))
									entityManager.template RemoveComponent<GravityComponent>(idA);
								if (entityManager.template HasComponent<MovementComponent>(idA))
									entityManager.template RemoveComponent<MovementComponent>(idA);
								if (entityManager.template HasComponent<GravityComponent>(idB))
									entityManager.template RemoveComponent<GravityComponent>(idB);
								if (entityManager.template HasComponent<MovementComponent>(idB))
									entityManager.template RemoveComponent<MovementComponent>(idB);
							}
							else
							{
								movementCompB.velocity = Point2D(0.0f, 0.0f);
								if (entityManager.template HasComponent<MovementComponent>(idA))
									entityManager.template GetComponent<MovementComponent>(idA).velocity = Point2D(0.0f, 0.0f);
							}
						}
					}
				}
			}

			return true;
		}

	private:
		void playerCollisionCheck(Scene<Components...> &scene, const CollisionFlags &collidedSides, const Point2D &preCollisionVelocity)
		{
			auto &entityManager = scene.GetEntityManager();

			// Determine if we should shake
			bool shouldShake = false;
			Point2D shakeDir(0.0f, 0.0f);
			float shakeMagnitude = 0.0f;

			// Wall collisions, only shake if hitting wall with large enough velocity
			if (collidedSides.left || collidedSides.right)
			{
				float horizontalVelocity = std::abs(preCollisionVelocity.x);
				const float minWallShakeVelocity = 50.0f;

				if (horizontalVelocity >= minWallShakeVelocity)
				{
					shouldShake = true;
					if (collidedSides.left)
					{
						shakeDir.x = -1.0f;
					}
					else if (collidedSides.right)
					{
						shakeDir.x = 1.0f;
					}

					float baseShake = 1.0f;
					float velocityMultiplier = 0.05f;
					shakeMagnitude = baseShake + (horizontalVelocity * velocityMultiplier);
				}
			}
			// Floor collision, only shake if we were falling (landing impact)
			else if (collidedSides.bottom)
			{
				const float minLandingShakeVelocity = 200.0f;
				if (preCollisionVelocity.y > minLandingShakeVelocity)
				{
					shouldShake = true;
					shakeDir.y = 1.0f;

					float verticalVelocity = preCollisionVelocity.y;
					float baseShake = 1.0f;
					float velocityMultiplier = 0.05f;
					shakeMagnitude = baseShake + (verticalVelocity * velocityMultiplier);
				}
			}
			// Ceiling collision shake if we hit it
			else if (collidedSides.top)
			{
				shouldShake = true;

				shakeDir.y = -1.0f;

				float verticalVelocity = std::abs(preCollisionVelocity.y);
				const float minCeilingShakeVelocity = 50.0f;
				if (verticalVelocity < minCeilingShakeVelocity)
				{
					verticalVelocity = minCeilingShakeVelocity;
				}
				float baseShake = 1.0f;
				float velocityMultiplier = 0.05f;
				shakeMagnitude = baseShake + (verticalVelocity * velocityMultiplier);
			}

			if (!shouldShake)
			{
				return;
			}

			// Find camera entity and apply shake
			for (auto camIt = entityManager.begin(); camIt != entityManager.end(); ++camIt)
			{
				EntityID camEntityId = camIt->getID();

				if (entityManager.template HasComponent<CameraComponent>(camEntityId))
				{
					CameraShake shake;
					shake.framesRemaining = 10;
					shake.magnitude = Point2D(shakeDir.x * shakeMagnitude,
											  shakeDir.y * shakeMagnitude);

					if (entityManager.template HasComponent<CameraShake>(camEntityId))
					{
						auto &existingShake = entityManager.template GetComponent<CameraShake>(camEntityId);
						if (shakeMagnitude > std::sqrt(existingShake.magnitude.x * existingShake.magnitude.x +
													   existingShake.magnitude.y * existingShake.magnitude.y))
						{
							existingShake = shake;
						}
					}
					else
					{
						entityManager.template AddComponent<CameraShake>(camEntityId, shake);
					}
					break;
				}
			}
		}

		/**
		 * @brief Resolves an AABB collision between two rectangles.
		 *
		 * @param a The first rectangle (will be modified to resolve collision)
		 * @param b The second rectangle (static reference)
		 * @param aPrev The previous position of rectangle a
		 * @param bPrev The previous position of rectangle b
		 * @param flagsA Collision flags for rectangle a
		 * @param flagsB Collision flags for rectangle b
		 * @param velocityA Optional velocity of rectangle a for collision direction determination
		 */
		void ResolveAABBCollision(Rect &a, const Rect &b,
										const Rect &aPrev, const Rect &bPrev,
										CollisionFlags &flagsA, CollisionFlags &flagsB,
										const Point2D &velocityA = Point2D(0, 0))
		{
			Point2D overlap = GetOverlap(a, b);
			if (overlap.x <= 0.0f || overlap.y <= 0.0f)
				return;

			// Small delta to add separation
			const float delta = 0.01f;

			// Compute center delta
			Point2D centerA = aPrev.topLeft + Point2D(aPrev.width / 2, aPrev.height / 2);
			Point2D centerB = bPrev.topLeft + Point2D(bPrev.width / 2, bPrev.height / 2);
			float dx = centerA.x - centerB.x;
			float dy = centerA.y - centerB.y;

			// Determine which axis to resolve
			bool resolveX = overlap.x < overlap.y;

			if (resolveX)
			{
				if (dx < 0.0f && velocityA.x >= 0.0f)
				{
					// Came from left, hit right side of B
					a.topLeft.x -= overlap.x + delta;
					flagsA.right = true;
					flagsB.left = true;
				}
				else if (dx > 0.0f && velocityA.x <= 0.0f)
				{
					// Came from right, hit left side of B
					a.topLeft.x += overlap.x + delta;
					flagsA.left = true;
					flagsB.right = true;
				}
			}
			else
			{
				if (dy < 0.0f && velocityA.y >= 0.0f)
				{
					// Came from above, landed on B
					a.topLeft.y -= overlap.y + delta;
					flagsA.bottom = true;
					flagsB.top = true;
				}
				else if (dy > 0.0f && velocityA.y <= 0.0f)
				{
					// Came from below, hit ceiling
					a.topLeft.y += overlap.y + delta;
					flagsA.top = true;
					flagsB.bottom = true;
				}
			}
		}
	};
} // namespace ECSEngine
