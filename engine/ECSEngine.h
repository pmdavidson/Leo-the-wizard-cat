#pragma once

#include <SFML/Graphics.hpp>
#include <SFML/Window.hpp>
#include <vector>
#include <string>
#include <unordered_map>
#include <random>
#include <cmath>
#include <iostream>
#include "MathUtil.h"
#include "EntityManager.h"
#include "SpriteManager.h"
#include "SoundManager.h"
#include "CameraComponent.h"
#include "InputComponent.h"
#include "LocationComponent.h"
#include "CollisionComponent.h"
#include "SpriteComponent.h"
#include "WindowManager.h"
#include "SpawnComponent.h"
#include "ScoreComponent.h"

static_assert(std::is_same_v<sf::IntRect, sf::Rect<int>>, "IntRect is not defined correctly.");

namespace ECSEngine
{

	/**
	 * @brief Main ECS engine class that manages all systems and resources.
	 *
	 * @tparam Components The component types that can be attached to entities.
	 */
	template <typename... Components>
	class ECSEngine
	{
	public:
		/**
		 * @brief Constructs the ECS engine and initializes the window.
		 *
		 * @param width Width of the render window in pixels.
		 * @param height Height of the render window in pixels.
		 * @param name Title of the render window.
		 */
		ECSEngine(unsigned int width, unsigned int height, const std::string &name);

		/**
		 * @brief Runs the main game loop until the window is closed.
		 */
		void Run();

		/**
		 * @brief Gets a reference to the SoundManager.
		 *
		 * @return SoundManager& Reference to the SoundManager instance.
		 *
		 * @note The returned reference is valid for the lifetime of the ECSEngine instance.
		 */
		SoundManager &GetSoundManager()
		{
			return mSoundManager;
		}

		/**
		 * @brief Gets a reference to the SpriteManager.
		 *
		 * @return SpriteManager& Reference to the SpriteManager instance.
		 *
		 * @note The returned reference is valid for the lifetime of the ECSEngine instance.
		 */
		SpriteManager &GetSpriteManager()
		{
			return mSpriteManager;
		}

		/**
		 * @brief Gets a reference to the EntityManager.
		 *
		 * @return EntityManager<Components...>& Reference to the EntityManager instance.
		 *
		 * @note The returned reference is valid for the lifetime of the ECSEngine instance.
		 */
		EntityManager<Components...> &GetEntityManager()
		{
			return mEntityManager;
		}

	private:
		// Systems
		void ProcessEvents();
		void CollisionSystemUpdate();
		void InputSystem();
		void GravitySystem();
		void MovementSystem();
		void CollisionSystem();
		void ScoreSystem();
		void CameraSystem();
		void SpriteSystem();
		void SpawnSystem();

		void playerCollisionCheck(const CollisionFlags &collidedSides, const Point2D &preCollisionVelocity)
		{
			// Determine if we should shake
			bool shouldShake = false;
			Point2D shakeDir(0.0f, 0.0f);
			float shakeMagnitude = 0.0f;

			// Wall collisions,  only shake if hitting wall with large enough velocity
			if (collidedSides.left || collidedSides.right)
			{
				// Use horizontal velocity for wall shake
				float horizontalVelocity = std::abs(preCollisionVelocity.x);
				const float minWallShakeVelocity = 50.0f;

				// Only shake if there's a large velocity towards the wall
				if (horizontalVelocity >= minWallShakeVelocity)
				{
					shouldShake = true;
					if (collidedSides.left)
					{
						shakeDir.x = -1.0f; // shake left
					}
					else if (collidedSides.right)
					{
						shakeDir.x = 1.0f; // shake right
					}

					float baseShake = 1.0f;
					float velocityMultiplier = 0.05f;
					shakeMagnitude = baseShake + (horizontalVelocity * velocityMultiplier);
				}
			}
			// Floor collision, only shake if we were falling (landing impact)
			else if (collidedSides.bottom)
			{
				// Only shake if we were falling (positive Y velocity means downward)
				// Use a threshold to prevent shake from small bounces or walking
				const float minLandingShakeVelocity = 200.0f; // Need substantial fall to shake
				if (preCollisionVelocity.y > minLandingShakeVelocity)
				{
					shouldShake = true;
					shakeDir.y = 1.0f; // shake down

					// Use vertical velocity for landing shake
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
				shakeDir.y = -1.0f; // shake up

				// Use upward velocity for ceiling shake
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
			for (auto camIt = mEntityManager.begin(); camIt != mEntityManager.end(); ++camIt)
			{
				EntityID camEntityId = camIt->getID();

				if (mEntityManager.template HasComponent<CameraComponent>(camEntityId))
				{
					// Add or update CameraShake component
					CameraShake shake;
					shake.framesRemaining = 10; // Shake for 10 frames
					shake.magnitude = Point2D(shakeDir.x * shakeMagnitude,
											  shakeDir.y * shakeMagnitude);

					if (mEntityManager.template HasComponent<CameraShake>(camEntityId))
					{
						// Update existing shake (use larger magnitude if already shaking)
						auto &existingShake = mEntityManager.template GetComponent<CameraShake>(camEntityId);
						if (shakeMagnitude > std::sqrt(existingShake.magnitude.x * existingShake.magnitude.x +
													   existingShake.magnitude.y * existingShake.magnitude.y))
						{
							existingShake = shake;
						}
					}
					else
					{
						// Add new shake component
						mEntityManager.template AddComponent<CameraShake>(camEntityId, shake);
					}
					break; // Only one camera entity
				}
			}
		}

		EntityManager<Components...> mEntityManager;
		SpriteManager mSpriteManager;
		SoundManager mSoundManager;
		WindowManager mWindowManager;
	};

	template <typename... Components>
	ECSEngine<Components...>::ECSEngine(unsigned int width, unsigned int height, const std::string &name)
		: mWindowManager(width, height, name)
	{
	}

	template <typename... Components>
	void ECSEngine<Components...>::Run()
	{
		sf::RenderWindow *window = mWindowManager.GetWindow();

		// Main game loop
		while (window->isOpen())
		{
			ProcessEvents();
			CollisionSystemUpdate();
			InputSystem();
			GravitySystem();
			MovementSystem();
			CollisionSystem();
			ScoreSystem();
			CameraSystem();
			SpriteSystem();
			SpawnSystem();
		}
	}

	template <typename... Components>
	void ECSEngine<Components...>::ProcessEvents()
	{
		sf::RenderWindow *window = mWindowManager.GetWindow();

		// Process all pending events
		while (auto event = window->pollEvent())
		{
			if (event->is<sf::Event::Closed>())
			{
				window->close();
			}
			else if (auto keyEvent = event->getIf<sf::Event::KeyPressed>())
			{
				// Convert Key to Scancode for InputComponent
				sf::Keyboard::Scancode scancode = sf::Keyboard::delocalize(keyEvent->code);

				// Only process valid scancodes
				if (scancode != sf::Keyboard::Scan::Unknown)
				{
					size_t scancodeIndex = static_cast<size_t>(scancode);
					// Check that scancode is within bitset range
					if (scancodeIndex < sf::Keyboard::ScancodeCount)
					{
						// Update input components for all entities with InputComponent
						for (auto it = mEntityManager.begin(); it != mEntityManager.end(); ++it)
						{
							if (!it->isActive())
								continue;
							EntityID entityId = it->getID();

							if (mEntityManager.template HasComponent<InputComponent>(entityId))
							{
								auto &inputComp = mEntityManager.template GetComponent<InputComponent>(entityId);
								inputComp.keydown.set(scancodeIndex);
							}
						}
					}
				}
			}
			else if (auto keyEvent = event->getIf<sf::Event::KeyReleased>())
			{
				// Convert Key to Scancode for InputComponent
				sf::Keyboard::Scancode scancode = sf::Keyboard::delocalize(keyEvent->code);

				// Only process valid scancodes
				if (scancode != sf::Keyboard::Scan::Unknown)
				{
					size_t scancodeIndex = static_cast<size_t>(scancode);
					// Bounds check to ensure scancode is within bitset range
					if (scancodeIndex < sf::Keyboard::ScancodeCount)
					{
						// Clear key when released
						for (auto it = mEntityManager.begin(); it != mEntityManager.end(); ++it)
						{
							if (!it->isActive())
								continue;
							EntityID entityId = it->getID();

							if (mEntityManager.template HasComponent<InputComponent>(entityId))
							{
								auto &inputComp = mEntityManager.template GetComponent<InputComponent>(entityId);
								inputComp.keydown.reset(scancodeIndex);
							}
						}
					}
				}
			}
		}
	}

	template <typename... Components>
	void ECSEngine<Components...>::CollisionSystemUpdate()
	{
		for (auto it = mEntityManager.begin(); it != mEntityManager.end(); ++it)
		{
			if (!it->isActive())
				continue;

			EntityID id = it->getID();

			// Check if an entity has a collision comp and location comp
			if (!mEntityManager.template HasComponent<CollisionComponent>(id) ||
				!mEntityManager.template HasComponent<LocationComponent>(id))
				continue;

			auto &collisionComp = mEntityManager.template GetComponent<CollisionComponent>(id);
			auto &locationComp = mEntityManager.template GetComponent<LocationComponent>(id);

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
	}

	template <typename... Components>
	void ECSEngine<Components...>::InputSystem()
	{
		// Process input for platformer controls (w/a/s/d/space)
		float deltaTime = 1.0f / 60.0f;

		for (auto it = mEntityManager.begin(); it != mEntityManager.end(); ++it)
		{
			if (!it->isActive())
				continue;
			EntityID entityId = it->getID();

			if (mEntityManager.template HasComponent<InputComponent>(entityId) &&
				mEntityManager.template HasComponent<MovementComponent>(entityId))
			{
				auto &inputComp = mEntityManager.template GetComponent<InputComponent>(entityId);
				auto &movementComp = mEntityManager.template GetComponent<MovementComponent>(entityId);

				// Platformer movement constants
				const float maxSpeed = 200.0f;			 // Maximum horizontal velocity
				const float acceleration = 800.0f;		 // Horizontal acceleration
				const float deceleration = 1000.0f;		 // Horizontal deceleration
				const float jumpVelocity = -400.0f;		 // Jump velocity (negative = up)
				const float wallJumpVelocityX = 300.0f;	 // Wall jump horizontal velocity
				const float wallJumpVelocityY = -350.0f; // Wall jump vertical velocity

				// Check for horizontal movement (A/D)
				sf::Keyboard::Scancode scancodeA = sf::Keyboard::delocalize(sf::Keyboard::Key::A);
				sf::Keyboard::Scancode scancodeD = sf::Keyboard::delocalize(sf::Keyboard::Key::D);

				bool movingLeft = false;
				if (scancodeA != sf::Keyboard::Scan::Unknown && static_cast<size_t>(scancodeA) < sf::Keyboard::ScancodeCount)
				{
					movingLeft = inputComp.keydown[static_cast<size_t>(scancodeA)];
				}

				bool movingRight = false;
				if (scancodeD != sf::Keyboard::Scan::Unknown && static_cast<size_t>(scancodeD) < sf::Keyboard::ScancodeCount)
				{
					movingRight = inputComp.keydown[static_cast<size_t>(scancodeD)];
				}

				// Check collision sides for wall jumping
				bool onGround = false;
				bool onWallLeft = false;
				bool onWallRight = false;
				bool wasFalling = false;

				// Track if we jumped recently for landing detection
				static std::unordered_map<EntityID, bool> recentlyJumped;

				if (mEntityManager.template HasComponent<CollisionComponent>(entityId))
				{
					auto &collisionComp = mEntityManager.template GetComponent<CollisionComponent>(entityId);
					onGround = collisionComp.collidedSides.bottom;
					onWallLeft = collisionComp.collidedSides.left;
					onWallRight = collisionComp.collidedSides.right;
					wasFalling = movementComp.velocity.y > 0;

					// Detect landing, if we're on ground and we recently jumped, play landing sound
					if (onGround && recentlyJumped[entityId])
					{
						mSoundManager.PlaySound("land");
						recentlyJumped[entityId] = false; // Clear the flag
					}

					// Play wall push sound when pushing against wall while falling
					static std::unordered_map<EntityID, bool> wasPushingWall;
					if ((onWallLeft || onWallRight) && wasFalling && !onGround)
					{
						// Only play if not already playing (ow my ears)
						bool wasPushingWallLastFrame = wasPushingWall[entityId];
						if (!wasPushingWallLastFrame)
						{
							mSoundManager.PlaySound("wall_push");
						}
						wasPushingWall[entityId] = true;
					}
					else
					{
						wasPushingWall[entityId] = false;
					}
				}

				// Handle horizontal movement with acceleration
				if (movingLeft && !movingRight)
				{
					// Accelerate left
					movementComp.velocity.x -= acceleration * deltaTime;
					if (movementComp.velocity.x < -maxSpeed)
						movementComp.velocity.x = -maxSpeed;
				}
				else if (movingRight && !movingLeft)
				{
					// Accelerate right
					movementComp.velocity.x += acceleration * deltaTime;
					if (movementComp.velocity.x > maxSpeed)
						movementComp.velocity.x = maxSpeed;
				}
				else
				{
					// Decelerate
					if (movementComp.velocity.x > 0)
					{
						movementComp.velocity.x -= deceleration * deltaTime;
						if (movementComp.velocity.x < 0)
							movementComp.velocity.x = 0;
					}
					else if (movementComp.velocity.x < 0)
					{
						movementComp.velocity.x += deceleration * deltaTime;
						if (movementComp.velocity.x > 0)
							movementComp.velocity.x = 0;
					}
				}

				// Handle jumping (W/Space), only if on ground
				sf::Keyboard::Scancode scancodeW = sf::Keyboard::delocalize(sf::Keyboard::Key::W);
				sf::Keyboard::Scancode scancodeSpace = sf::Keyboard::delocalize(sf::Keyboard::Key::Space);

				bool wPressed = false;
				if (scancodeW != sf::Keyboard::Scan::Unknown && static_cast<size_t>(scancodeW) < sf::Keyboard::ScancodeCount)
				{
					wPressed = inputComp.keydown[static_cast<size_t>(scancodeW)];
				}

				bool spacePressed = false;
				if (scancodeSpace != sf::Keyboard::Scan::Unknown && static_cast<size_t>(scancodeSpace) < sf::Keyboard::ScancodeCount)
				{
					spacePressed = inputComp.keydown[static_cast<size_t>(scancodeSpace)];
				}

				bool jumpPressed = wPressed || spacePressed;

				static std::unordered_map<EntityID, bool> wasJumpPressed;
				bool wasJumpPressedLastFrame = wasJumpPressed[entityId];
				wasJumpPressed[entityId] = jumpPressed;

				if (jumpPressed && !wasJumpPressedLastFrame)
				{
					if (onGround)
					{
						// Normal jump
						movementComp.velocity.y = jumpVelocity;
						mSoundManager.PlaySound("jump");
						recentlyJumped[entityId] = true; // Mark that we recently jumped
					}
					else if (wasFalling && (onWallLeft || onWallRight))
					{
						// Wall jump
						if (onWallLeft)
						{
							movementComp.velocity.x = wallJumpVelocityX;
						}
						else if (onWallRight)
						{
							movementComp.velocity.x = -wallJumpVelocityX;
						}
						movementComp.velocity.y = wallJumpVelocityY;
						mSoundManager.PlaySound("wall_push");
						recentlyJumped[entityId] = true; // Mark that we recently jumped
					}
				}

				// Handle fast fall (S key while falling)
				sf::Keyboard::Scancode scancodeS = sf::Keyboard::delocalize(sf::Keyboard::Key::S);
				bool downPressed = (scancodeS != sf::Keyboard::Scan::Unknown &&
									static_cast<size_t>(scancodeS) < sf::Keyboard::ScancodeCount)
									   ? inputComp.keydown[static_cast<size_t>(scancodeS)]
									   : false;
				if (downPressed && !onGround && movementComp.velocity.y > 0)
				{
					// Increase fall speed
					movementComp.velocity.y += 200.0f * deltaTime; // Additional downward acceleration
				}
			}
		}
	}

	template <typename... Components>
	void ECSEngine<Components...>::GravitySystem()
	{
		float deltaTime = 1.0f / 60.0f; // assuming 60FPS?

		for (auto it = mEntityManager.begin(); it != mEntityManager.end(); ++it)
		{
			if (!it->isActive())
				continue;
			EntityID entityId = it->getID();

			if (mEntityManager.template HasComponent<GravityComponent>(entityId) &&
				mEntityManager.template HasComponent<MovementComponent>(entityId))
			{
				auto &gravityComp = mEntityManager.template GetComponent<GravityComponent>(entityId);
				auto &movementComp = mEntityManager.template GetComponent<MovementComponent>(entityId);

				// Check if player is on wall while falling
				bool onWallWhileFalling = false;
				if (mEntityManager.template HasComponent<InputComponent>(entityId) &&
					mEntityManager.template HasComponent<CollisionComponent>(entityId))
				{
					auto &collisionComp = mEntityManager.template GetComponent<CollisionComponent>(entityId);
					bool onWall = collisionComp.collidedSides.left || collisionComp.collidedSides.right;
					bool falling = movementComp.velocity.y > 0;
					onWallWhileFalling = onWall && falling;
				}

				// Apply gravity, reduced if sliding on wall
				float gravityMultiplier = onWallWhileFalling ? 0.3f : 1.0f;
				movementComp.velocity += gravityComp.acceleration * deltaTime * gravityMultiplier;
			}
		}
	}

	template <typename... Components>
	void ECSEngine<Components...>::MovementSystem()
	{
		float deltaTime = 1.0f / 60.0f; // assuming 60FPS?

		// Update locations based on velocity
		for (auto it = mEntityManager.begin(); it != mEntityManager.end(); ++it)
		{
			if (!it->isActive())
				continue;
			EntityID entityId = it->getID();

			if (mEntityManager.template HasComponent<LocationComponent>(entityId) &&
				mEntityManager.template HasComponent<MovementComponent>(entityId))
			{
				auto &locationComp = mEntityManager.template GetComponent<LocationComponent>(entityId);
				auto &movementComp = mEntityManager.template GetComponent<MovementComponent>(entityId);

				// Update position based on velocity
				locationComp.position.x += movementComp.velocity.x * deltaTime;
				locationComp.position.y += movementComp.velocity.y * deltaTime;
			}
		}
	}

	template <typename... Components>
	void ECSEngine<Components...>::CollisionSystem()
	{
		auto &entityManager = GetEntityManager();

		// Update all current bounds after movement and clear collision flags
		for (auto it = entityManager.begin(); it != entityManager.end(); ++it)
		{
			if (!it->isActive())
				continue;
			EntityID id = it->getID();

			// Skip entities without required components
			if (!entityManager.template HasComponent<CollisionComponent>(id) ||
				!entityManager.template HasComponent<LocationComponent>(id))
				continue;

			auto &collisionComp = entityManager.template GetComponent<CollisionComponent>(id);
			auto &locationComp = entityManager.template GetComponent<LocationComponent>(id);

			// Update current bounds to match position after movement
			collisionComp.currentBounds.topLeft = locationComp.position + collisionComp.localBounds.topLeft;
			collisionComp.currentBounds.width = collisionComp.localBounds.width;
			collisionComp.currentBounds.height = collisionComp.localBounds.height;

			// Clear collision flags for this frame
			collisionComp.collidedSides = {};
		}

		// Check and resolve collisions
		for (auto itA = entityManager.begin(); itA != entityManager.end(); ++itA)
		{
			auto &entityA = *itA;
			if (!entityA.isActive())
				continue;
			EntityID idA = entityA.getID();

			// Skip entities without required components
			if (!entityManager.template HasComponent<CollisionComponent>(idA) ||
				!entityManager.template HasComponent<LocationComponent>(idA))
				continue;

			auto &colA = entityManager.template GetComponent<CollisionComponent>(idA);
			auto &LocA = entityManager.template GetComponent<LocationComponent>(idA);

			// Check against all remaining entities
			for (auto itB = std::next(itA); itB != entityManager.end(); ++itB)
			{
				auto &entityB = *itB;
				if (!entityB.isActive())
					continue;
				EntityID idB = entityB.getID();

				// Skip entities without required components
				if (!entityManager.template HasComponent<CollisionComponent>(idB) ||
					!entityManager.template HasComponent<LocationComponent>(idB))
					continue;

				auto &colB = entityManager.template GetComponent<CollisionComponent>(idB);
				auto &LocB = entityManager.template GetComponent<LocationComponent>(idB);

				// Skip collision between two static objects
				if (colA.isStatic && colB.isStatic)
					continue;

				Rect BoundsA(colA.currentBounds.topLeft, colA.currentBounds.width, colA.currentBounds.height);
				Rect BoundsB(colB.currentBounds.topLeft, colB.currentBounds.width, colB.currentBounds.height);

				// Skip if entities don't intersect
				if (!BoundsA.intersects(BoundsB))
					continue;

				// Identify entity types for collision handling
				bool isPlayerA = entityManager.template HasComponent<InputComponent>(idA);
				bool isPlayerB = entityManager.template HasComponent<InputComponent>(idB);
				bool isSpawnerA = entityManager.template HasComponent<SpawnComponent>(idA);
				bool isSpawnerB = entityManager.template HasComponent<SpawnComponent>(idB);

				// Stars are non-static entities with gravity or no movement component
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

				// Resolve collision based on which entity is static
				if (colB.isStatic)
				{
					// Entity A is dynamic, B is static - resolve A's position
					ResolveAABBCollision(
						BoundsA, BoundsB,
						colA.previousBounds, colB.previousBounds,
						colA.collidedSides, colB.collidedSides, preCollisionVelocityA);

					LocA.position = BoundsA.topLeft - colA.localBounds.topLeft;
					colA.currentBounds = BoundsA;

					// Stop player movement on collision
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
					// Entity B is dynamic, A is static - resolve B's position
					ResolveAABBCollision(
						BoundsB, BoundsA,
						colB.previousBounds, colA.previousBounds,
						colB.collidedSides, colA.collidedSides, preCollisionVelocityB);

					LocB.position = BoundsB.topLeft - colB.localBounds.topLeft;
					colB.currentBounds = BoundsB;

					// Stop player movement on collision
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
					// Both entities are dynamic (e.g., star-to-star collisions)
					// Resolve both entities' positions
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

				// Trigger camera shake and other player collision effects
				if (isPlayerA && colB.isStatic)
				{
					playerCollisionCheck(colA.collidedSides, preCollisionVelocityA);
				}
				else if (isPlayerB && colA.isStatic)
				{
					playerCollisionCheck(colB.collidedSides, preCollisionVelocityB);
				}

				bool removedA = false;
				bool removedB = false;

				// Handle player-star collisions (star collection)
				if ((isPlayerA && isStarB) || (isPlayerB && isStarA))
				{
					// Play collection sound
					mSoundManager.PlaySound("star_collect");

					// Add score (10 points for star)
					for (auto scoreIt = entityManager.begin(); scoreIt != entityManager.end(); ++scoreIt)
					{
						EntityID scoreEntityId = scoreIt->getID();
						if (entityManager.template HasComponent<ScoreComponent>(scoreEntityId))
						{
							entityManager.template GetComponent<ScoreComponent>(scoreEntityId).currentScore += 10;
							break;
						}
					}

					// Remove the star entity
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

					// Star vs static object: bounce and lose speed
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
					// Star vs star collision
					else if (!removedB && isStarB)
					{
						bool isVerticalCollision = colA.collidedSides.top || colA.collidedSides.bottom ||
												   colB.collidedSides.top || colB.collidedSides.bottom;

						// Vertical collision, stars stick together
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
						// Horizontal collision, stop both stars
						else
						{
							movementCompA.velocity = Point2D(0.0f, 0.0f);
							if (entityManager.template HasComponent<MovementComponent>(idB))
								entityManager.template GetComponent<MovementComponent>(idB).velocity = Point2D(0.0f, 0.0f);
						}
					}
				}

				// Handle star B collisions
				else if (!removedB && isStarB && entityManager.template HasComponent<MovementComponent>(idB))
				{
					auto &movementCompB = entityManager.template GetComponent<MovementComponent>(idB);

					// Star vs static object
					if (colA.isStatic)
					{
						const float speedLoss = 0.75f;

						// Reverse horizontal velocity on wall hit
						if (colB.collidedSides.left || colB.collidedSides.right)
							movementCompB.velocity.x = -movementCompB.velocity.x * speedLoss;
						// Reverse vertical velocity on floor/ceiling hit
						if (colB.collidedSides.top || colB.collidedSides.bottom)
						{
							movementCompB.velocity.y = -movementCompB.velocity.y * speedLoss;
						}
					}
					// Star vs star collision
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
	}

	template <typename... Components>
	void ECSEngine<Components...>::ScoreSystem()
	{
		// Update score display entities
		for (auto it = mEntityManager.begin(); it != mEntityManager.end(); ++it)
		{
			if (!it->isActive())
				continue;
			EntityID entityId = it->getID();

			if (mEntityManager.template HasComponent<ScoreComponent>(entityId))
			{
				auto &scoreComp = mEntityManager.template GetComponent<ScoreComponent>(entityId);

				std::string scoreStr = std::to_string(scoreComp.currentScore);
				if (scoreStr.length() < 3)
					scoreStr = std::string(3 - scoreStr.length(), '0') + scoreStr;
				if (scoreStr.length() > 3)
					scoreStr = scoreStr.substr(scoreStr.length() - 3);

				for (size_t i = 0; i < scoreComp.displayEntityIds.size() && i < 3; ++i)
				{
					EntityID displayEntityId = scoreComp.displayEntityIds[i];
					if (mEntityManager.template HasComponent<SpriteComponent>(displayEntityId))
					{
						int digit = scoreStr[i] - '0';
						if (digit >= 0 && digit < 10 && digit < static_cast<int>(scoreComp.digitSpriteIds.size()))
						{
							auto &spriteComp = mEntityManager.template GetComponent<SpriteComponent>(displayEntityId);
							spriteComp.spriteId = scoreComp.digitSpriteIds[digit];
						}
					}
				}
			}
		}
	}

	template <typename... Components>
	void ECSEngine<Components...>::CameraSystem()
	{
		float deltaTime = 1.0f / 60.0f; // assuming 60FPS

		for (auto it = mEntityManager.begin(); it != mEntityManager.end(); ++it)
		{
			if (!it->isActive())
				continue;
			EntityID entityId = it->getID();

			if (mEntityManager.template HasComponent<CameraFollower>(entityId))
			{
				auto &cameraFollower = mEntityManager.template GetComponent<CameraFollower>(entityId);
				EntityID trackedEntityId = cameraFollower.entityId;

				// Find camera entity
				for (auto camIt = mEntityManager.begin(); camIt != mEntityManager.end(); ++camIt)
				{
					if (!camIt->isActive())
						continue;
					EntityID camEntityId = camIt->getID();

					if (mEntityManager.template HasComponent<CameraComponent>(camEntityId))
					{
						auto &cameraComp = mEntityManager.template GetComponent<CameraComponent>(camEntityId);

						// Get tracked entity's location and movement
						if (mEntityManager.template HasComponent<LocationComponent>(trackedEntityId))
						{
							auto &trackedLocation = mEntityManager.template GetComponent<LocationComponent>(trackedEntityId);

							// Get window size
							sf::RenderWindow *window = mWindowManager.GetWindow();
							sf::Vector2u windowSize = window->getSize();
							float windowWidth = static_cast<float>(windowSize.x);

							float playerWindowX = (trackedLocation.position.x - cameraComp.position.x) / cameraComp.scale + windowWidth * 0.5f;

							// Calculate player's position as percentage of window width (0.0 to 1.0)
							float playerXPercent = playerWindowX / windowWidth;

							// Check if player is moving
							bool isMoving = false;
							if (mEntityManager.template HasComponent<MovementComponent>(trackedEntityId))
							{
								auto &movementComp = mEntityManager.template GetComponent<MovementComponent>(trackedEntityId);
								isMoving = std::abs(movementComp.velocity.x) > 0.1f; // Small threshold for "stopped"
							}

							// Calculate target camera X position
							float targetCameraX = cameraComp.position.x;
							float trackingSpeed = 0.0f; 

							// Determine which zone player is in and calculate target camera position
							if (playerXPercent < 0.1f)
							{
								// Left outer 10%
								targetCameraX = trackedLocation.position.x - (windowWidth * 0.1f - windowWidth * 0.5f) * cameraComp.scale;
								trackingSpeed = 0.0f; // Instant follow
							}
							else if (playerXPercent > 0.9f)
							{
								// Right outer 10%
								targetCameraX = trackedLocation.position.x - (windowWidth * 0.9f - windowWidth * 0.5f) * cameraComp.scale;
								trackingSpeed = 0.0f; // Instant follow
							}
							else if (playerXPercent < 0.3f)
							{
								// Left 10-30% zone
								targetCameraX = trackedLocation.position.x - (windowWidth * 0.3f - windowWidth * 0.5f) * cameraComp.scale;
								trackingSpeed = 5.0f; 
							}
							else if (playerXPercent > 0.7f)
							{
								// Right 10-30% zone
								targetCameraX = trackedLocation.position.x - (windowWidth * 0.7f - windowWidth * 0.5f) * cameraComp.scale;
								trackingSpeed = 5.0f;
							}
							else
							{
								// Center 40%
								if (!isMoving)
								{
									// Check if player is outside center 40% and adjust toward center
									float centerWindowX = windowWidth * 0.5f;
									float distanceFromCenter = playerWindowX - centerWindowX;

									// If outside center 40%, move toward center
									if (std::abs(distanceFromCenter) > windowWidth * 0.2f) // 20% from center = edge of 40% zone
									{
										// playerWindowX = windowWidth * 0.5 (center)
										targetCameraX = trackedLocation.position.x - (windowWidth * 0.5f - windowWidth * 0.5f) * cameraComp.scale;
										targetCameraX = trackedLocation.position.x;
										trackingSpeed = 3.0f; // Slower adjustment when stopped
									}
									else
									{
										// Player is in center 40%, no adjustment needed
										targetCameraX = cameraComp.position.x;
									}
								}
								else
								{
									// If moving and in dead zone, no camera movement
									targetCameraX = cameraComp.position.x;
								}
							}

							// Apply camera adjustment with interpolation
							float cameraDeltaX = targetCameraX - cameraComp.position.x;
							if (std::abs(cameraDeltaX) > 0.01f) // Small threshold to prevent jitter
							{
								if (trackingSpeed == 0.0f)
								{
									// Instant follow
									cameraComp.position.x = targetCameraX;
								}
								else
								{
									// EMA
									float lerpFactor = 1.0f - std::exp(-trackingSpeed * deltaTime);
									cameraComp.position.x += cameraDeltaX * lerpFactor;
								}
							}

							// Camera Y stays fixed at level position
							const float levelY = 512.0f;
							cameraComp.position.y = levelY;

							// Apply camera shake if present
							if (mEntityManager.template HasComponent<CameraShake>(camEntityId))
							{
								auto &shake = mEntityManager.template GetComponent<CameraShake>(camEntityId);

								// Apply random shake offset
								static std::random_device rd;
								static std::mt19937 gen(rd());
								std::uniform_real_distribution<float> shakeDist(-1.0f, 1.0f);

								float shakeX = shakeDist(gen) * shake.magnitude.x;
								float shakeY = shakeDist(gen) * shake.magnitude.y;

								cameraComp.position.x += shakeX;
								cameraComp.position.y += shakeY;

								// Decrement frames remaining
								shake.framesRemaining--;
								if (shake.framesRemaining <= 0)
								{
									mEntityManager.template RemoveComponent<CameraShake>(camEntityId);
								}
							}

							// Update WindowManager camera
							mWindowManager.SetCamera(cameraComp.position);
							mWindowManager.SetWorldScale(cameraComp.scale);
						}
						break;
					}
				}
			}
		}
	}

	template <typename... Components>
	void ECSEngine<Components...>::SpriteSystem()
	{
		auto &entityManager = GetEntityManager();
		auto &spriteManager = GetSpriteManager();
		sf::RenderWindow *window = mWindowManager.GetWindow();

		window->clear();

		// Iterate through all entities and render sprites
		for (auto it = entityManager.begin(); it != entityManager.end(); ++it)
		{
			if (!it->isActive())
				continue;
			EntityID entityId = it->getID();

			// Only render entities with sprite components
			if (entityManager.template HasComponent<SpriteComponent>(entityId))
			{
				auto &spriteComp = entityManager.template GetComponent<SpriteComponent>(entityId);

				sf::Sprite &sprite = spriteManager.GetSprite(spriteComp.spriteId);
				Point2D drawPos;

				if (spriteComp.inWorldSpace && entityManager.template HasComponent<LocationComponent>(entityId))
				{
					auto &locationComp = entityManager.template GetComponent<LocationComponent>(entityId);
					Point2D worldPos = {
						locationComp.position.x + spriteComp.bounds.topLeft.x,
						locationComp.position.y + spriteComp.bounds.topLeft.y};
					drawPos = mWindowManager.WorldToWindow(worldPos);
				}
				else if (entityManager.template HasComponent<LocationComponent>(entityId))
				{
					auto &locationComp = entityManager.template GetComponent<LocationComponent>(entityId);
					drawPos = Point2D(
						locationComp.position.x + spriteComp.bounds.topLeft.x,
						locationComp.position.y + spriteComp.bounds.topLeft.y);
				}
				else
				{
					drawPos = spriteComp.bounds.topLeft;
				}

				sprite.setPosition(sf::Vector2f(drawPos.x, drawPos.y));
				window->draw(sprite);
			}
		}

		window->display();
	}

	template <typename... Components>
	void ECSEngine<Components...>::SpawnSystem()
	{	
		// Random number gen, starMovement.velocity = Point2D(std::cos(angle) * speed, std::sin(angle) * speed);
		float deltaTime = 1.0f / 60.0f;
		static std::random_device rd;
		static std::mt19937 gen(rd());
		static std::uniform_real_distribution<float> angleDist(0.0f, 2.0f * 3.14159f); // (0, 2pi)
		static std::uniform_real_distribution<float> speedDist(50.0f, 150.0f); // (50, 150)

		for (auto it = mEntityManager.begin(); it != mEntityManager.end(); ++it)
		{
			if (!it->isActive())
				continue;
			EntityID entityId = it->getID();

			if (mEntityManager.template HasComponent<SpawnComponent>(entityId))
			{
				auto &spawnComp = mEntityManager.template GetComponent<SpawnComponent>(entityId);

				// Decrease time to next spawn
				spawnComp.timeToNextSpawn -= deltaTime;

				// Check if it's time to spawn
				if (spawnComp.timeToNextSpawn <= 0.0f && spawnComp.totalSpawnEvents > 0)
				{
					// Ensure spawner has a location component to determine spawn position
					if (mEntityManager.template HasComponent<LocationComponent>(entityId))
					{
						// Get spawner's position and sprite information
						auto &spawnerLocation = mEntityManager.template GetComponent<LocationComponent>(entityId);
						auto &spriteManager = GetSpriteManager();
						sf::Sprite &sprite = spriteManager.GetSprite(spawnComp.spriteId);
						sf::IntRect textureRect = sprite.getTextureRect();

						// Calculate sprite bounds from texture dimensions
						Rect spriteBounds(0.0f, 0.0f,
										  static_cast<float>(textureRect.size.x),
										  static_cast<float>(textureRect.size.y));

						Rect collisionBounds = {0.0f, 0.0f, spawnComp.tileW, spawnComp.tileH};

						EntityID starId = mEntityManager.CreateEntity("star");

						mEntityManager.template AddComponent<LocationComponent>(starId, LocationComponent(spawnerLocation.position));

						// Generate random angle and speed for star movement
						float angle = angleDist(gen);
						float speed = speedDist(gen);
						MovementComponent starMovement;

						// Calculate velocity vector from angle and speed
						starMovement.velocity = Point2D(std::cos(angle) * speed, std::sin(angle) * speed);
						mEntityManager.template AddComponent<MovementComponent>(starId, starMovement);

						// Apply gravity to the star
						mEntityManager.template AddComponent<GravityComponent>(starId, GravityComponent(Point2D(0, 600.0f)));

						// Set up collision component with initial bounds
						CollisionComponent starCollision(collisionBounds, false);
						starCollision.currentBounds = collisionBounds;
						starCollision.previousBounds = collisionBounds;
						mEntityManager.template AddComponent<CollisionComponent>(starId, starCollision);

						// Set up sprite component for rendering
						SpriteComponent starSprite;
						starSprite.spriteId = spawnComp.spriteId;
						starSprite.bounds = spriteBounds;
						starSprite.inWorldSpace = true;
						mEntityManager.template AddComponent<SpriteComponent>(starId, starSprite);

						// Reset spawn timer and decrement remaining spawn events
						spawnComp.timeToNextSpawn = spawnComp.timeBetweenSpawns;
						spawnComp.totalSpawnEvents--;
					}
				}
			}
		}
	}

	inline void ResolveAABBCollision(Rect &a, const Rect &b,
									 const Rect &aPrev, const Rect &bPrev,
									 CollisionFlags &flagsA, CollisionFlags &flagsB,
									 const Point2D &velocityA = Point2D(0, 0)) // optional
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

}