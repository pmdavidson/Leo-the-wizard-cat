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
#include <cmath>
#include <random>

namespace ECSEngine
{

	template <typename... Components>
	class CollisionSystem : public System<Components...>
	{
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

					bool isStarA = !isPlayerA && !isSpawnerA &&
								   entityManager.template HasComponent<CollisionComponent>(idA) &&
								   !entityManager.template GetComponent<CollisionComponent>(idA).isStatic &&
								   (entityManager.template HasComponent<GravityComponent>(idA) ||
									!entityManager.template HasComponent<MovementComponent>(idA));
					bool isStarB = !isPlayerB && !isSpawnerB &&
								   entityManager.template HasComponent<CollisionComponent>(idB) &&
								   !entityManager.template GetComponent<CollisionComponent>(idB).isStatic &&
								   (entityManager.template HasComponent<GravityComponent>(idB) ||
									!entityManager.template HasComponent<MovementComponent>(idB));

					Point2D preCollisionVelocityA(0.0f, 0.0f);
					Point2D preCollisionVelocityB(0.0f, 0.0f);

					if (isPlayerA && entityManager.template HasComponent<MovementComponent>(idA))
						preCollisionVelocityA = entityManager.template GetComponent<MovementComponent>(idA).velocity;
					if (isPlayerB && entityManager.template HasComponent<MovementComponent>(idB))
						preCollisionVelocityB = entityManager.template GetComponent<MovementComponent>(idB).velocity;

					// Resolve collision
					if (colB.isStatic)
					{
						ResolveAABBCollision(
							BoundsA, BoundsB,
							colA.previousBounds, colB.previousBounds,
							colA.collidedSides, colB.collidedSides, preCollisionVelocityA);

						LocA.position = BoundsA.topLeft - colA.localBounds.topLeft;
						colA.currentBounds = BoundsA;

						if (isPlayerA && entityManager.template HasComponent<MovementComponent>(idA))
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

						if (isPlayerB && entityManager.template HasComponent<MovementComponent>(idB))
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
	};

} // namespace ECSEngine
