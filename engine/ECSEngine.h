#pragma once

#include <SFML/Graphics.hpp>
#include <SFML/Window.hpp>
#include <vector>
#include <string>
#include <unordered_map>
#include <random>
#include <cmath>
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

		void playerCollisionCheck(EntityID playerId, const CollisionFlags &collidedSides)
		{
			if (mEntityManager.template HasComponent<MovementComponent>(playerId))
			{
				auto &movementComp = mEntityManager.template GetComponent<MovementComponent>(playerId);

				// Calculate camera shake magnitude using velocity
				float velocityMagnitude = std::sqrt(movementComp.velocity.x * movementComp.velocity.x +
													movementComp.velocity.y * movementComp.velocity.y);
				float baseShake = 1.0f;			 // Base shake amount (reduced for less intensity)
				float velocityMultiplier = 0.05f; // How much velocity affects shake (reduced for less intensity)
				float shakeMagnitude = baseShake + (velocityMagnitude * velocityMultiplier);

				// Determine direction of shake based on collision side
				Point2D shakeDir(0.0f, 0.0f);
				if (collidedSides.left)
				{
					shakeDir.x = -1.0f; // shake left
				}
				else if (collidedSides.right)
				{
					shakeDir.x = 1.0f; // shake right
				}

				if (collidedSides.top)
				{
					shakeDir.y = -1.0f; // shake up
				}
				else if (collidedSides.bottom)
				{
					shakeDir.y = 1.0f; // shake down
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

			if (!mEntityManager.template HasComponent<CollisionComponent>(id) ||
				!mEntityManager.template HasComponent<LocationComponent>(id))
				continue;

			auto& collisionComp = mEntityManager.template GetComponent<CollisionComponent>(id);
			auto& locationComp  = mEntityManager.template GetComponent<LocationComponent>(id);

			// STATIC OBJECTS STILL NEED WORLD-BOUNDS
			collisionComp.currentBounds.topLeft =
				locationComp.position + collisionComp.localBounds.topLeft;

			collisionComp.currentBounds.width  = collisionComp.localBounds.width;
			collisionComp.currentBounds.height = collisionComp.localBounds.height;

			// ONLY dynamic objects need to store previousBounds
			if (!collisionComp.isStatic)
			{
				collisionComp.previousBounds = collisionComp.currentBounds;
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
				bool movingLeft = (scancodeA != sf::Keyboard::Scan::Unknown &&
								   static_cast<size_t>(scancodeA) < sf::Keyboard::ScancodeCount)
									  ? inputComp.keydown[static_cast<size_t>(scancodeA)]
									  : false;
				bool movingRight = (scancodeD != sf::Keyboard::Scan::Unknown &&
									static_cast<size_t>(scancodeD) < sf::Keyboard::ScancodeCount)
									   ? inputComp.keydown[static_cast<size_t>(scancodeD)]
									   : false;

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

					// Detect landing: if we're on ground and we recently jumped, play landing sound
					if (onGround && recentlyJumped[entityId])
					{
						mSoundManager.PlaySound("land");
						recentlyJumped[entityId] = false; // Clear the flag
					}

					// Play wall push sound when pushing against wall while falling
					static std::unordered_map<EntityID, bool> wasPushingWall;
					if ((onWallLeft || onWallRight) && wasFalling && !onGround)
					{
						// Only play if not already playing
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

				// Handle jumping (W/Space) - only if on ground
				sf::Keyboard::Scancode scancodeW = sf::Keyboard::delocalize(sf::Keyboard::Key::W);
				sf::Keyboard::Scancode scancodeSpace = sf::Keyboard::delocalize(sf::Keyboard::Key::Space);
				bool wPressed = (scancodeW != sf::Keyboard::Scan::Unknown &&
								 static_cast<size_t>(scancodeW) < sf::Keyboard::ScancodeCount)
									? inputComp.keydown[static_cast<size_t>(scancodeW)]
									: false;
				bool spacePressed = (scancodeSpace != sf::Keyboard::Scan::Unknown &&
									 static_cast<size_t>(scancodeSpace) < sf::Keyboard::ScancodeCount)
										? inputComp.keydown[static_cast<size_t>(scancodeSpace)]
										: false;
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

				// Check for wall push while falling
				if (mEntityManager.template HasComponent<InputComponent>(entityId) &&
					mEntityManager.template HasComponent<CollisionComponent>(entityId))
				{
					auto &collisionComp = mEntityManager.template GetComponent<CollisionComponent>(entityId);
					bool onWall = collisionComp.collidedSides.left ||
								  collisionComp.collidedSides.right;
					bool falling = movementComp.velocity.y > 0;

					//is this supposed to be for wall jumping?
				}

				// Update position based on velocity
				locationComp.position.x += movementComp.velocity.x * deltaTime;
				locationComp.position.y += movementComp.velocity.y * deltaTime;

				if (mEntityManager.template HasComponent<CollisionComponent>(entityId)){

					auto &collisionComp = mEntityManager.template GetComponent<CollisionComponent>(entityId);
				
					collisionComp.currentBounds.topLeft = locationComp.position + collisionComp.localBounds.topLeft;
					collisionComp.currentBounds.width   = collisionComp.localBounds.width;
					collisionComp.currentBounds.height  = collisionComp.localBounds.height;
				}
			}
		}
	}

	template <typename... Components>
	void ECSEngine<Components...>::CollisionSystem()
	{
		auto &entityManager = GetEntityManager();

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

			colA.collidedSides = {};

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

				// colB.collidedSides = {};

				Rect BoundsA(colA.currentBounds.topLeft, colA.currentBounds.width, colA.currentBounds.height);
				Rect BoundsB(colB.currentBounds.topLeft, colB.currentBounds.width, colB.currentBounds.height);


				if (!BoundsA.intersects(BoundsB))
					continue;

				if (BoundsA.intersects(BoundsB))
				{
					std::cout << "=== COLLISION DETECTED ===\n";

					std::cout << "Player Loc: " 
							<< LocA.position.x << ", " << LocA.position.y << "\n";

					std::cout << "A localBounds: ("
							<< colA.localBounds.topLeft.x << ", " << colA.localBounds.topLeft.y
							<< ", w=" << colA.localBounds.width
							<< ", h=" << colA.localBounds.height << ")\n";

					std::cout << "A currentBounds: ("
							<< colA.currentBounds.topLeft.x << ", " << colA.currentBounds.topLeft.y
							<< ", w=" << colA.currentBounds.width
							<< ", h=" << colA.currentBounds.height << ")\n";

					std::cout << "B currentBounds: ("
							<< colB.currentBounds.topLeft.x << ", " << colB.currentBounds.topLeft.y
							<< ", w=" << colB.currentBounds.width
							<< ", h=" << colB.currentBounds.height << ")\n";

					auto overlap = GetOverlap(BoundsA, BoundsB);
					std::cout << "Overlap: (" << overlap.x << ", " << overlap.y << ")\n";
				}


				// if B is static then A is a star/player, and vice-versa
				if (colB.isStatic)
				{
					// ResolveAABBCollision(BoundsA, BoundsB, colA.collidedSides, colB.collidedSides);

					ResolveAABBCollision(
						BoundsA, BoundsB,
						colA.previousBounds, colB.previousBounds,
						colA.collidedSides, colB.collidedSides);

					LocA.position = BoundsA.topLeft - colA.localBounds.topLeft;
					colA.currentBounds = BoundsA;					
					
					// 💡 Add this block to stop movement
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
					// ResolveAABBCollision(BoundsB, BoundsA, colB.collidedSides, colA.collidedSides);

					ResolveAABBCollision(
						BoundsB, BoundsA,
						colB.previousBounds, colA.previousBounds,
						colB.collidedSides, colA.collidedSides);

					LocB.position = BoundsB.topLeft - colB.localBounds.topLeft;
					colB.currentBounds = BoundsB;

					// Stop velocity
					if (entityManager.template HasComponent<MovementComponent>(idB))
					{
						auto& vel = entityManager.template GetComponent<MovementComponent>(idB);
						if (colB.collidedSides.bottom || colB.collidedSides.top)
							vel.velocity.y = 0;
						if (colB.collidedSides.left || colB.collidedSides.right)
							vel.velocity.x = 0;
					}
				}

				// Check if player (idA) hit a solid object (idB)
				if (entityManager.template HasComponent<InputComponent>(idA) && colB.isStatic)
				{
					playerCollisionCheck(idA, colA.collidedSides);
				}
				if (entityManager.template HasComponent<InputComponent>(idB) && colA.isStatic)
				{
					playerCollisionCheck(idB, colB.collidedSides);
				}

				// Check if player collected a star (works for both idA=player/idB=star and idB=player/idA=star)
				// Stars have GravityComponent and CollisionComponent but not InputComponent
				bool isPlayerA = entityManager.template HasComponent<InputComponent>(idA);
				bool isPlayerB = entityManager.template HasComponent<InputComponent>(idB);
				bool isStarA = !isPlayerA &&
							   entityManager.template HasComponent<GravityComponent>(idA) &&
							   entityManager.template HasComponent<CollisionComponent>(idA);
				bool isStarB = !isPlayerB &&
							   entityManager.template HasComponent<GravityComponent>(idB) &&
							   entityManager.template HasComponent<CollisionComponent>(idB);

				// Track if we removed an entity (to skip collision handling for removed entities)
				bool removedA = false;
				bool removedB = false;

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
							auto &scoreComp = entityManager.template GetComponent<ScoreComponent>(scoreEntityId);
							scoreComp.currentScore += 10;
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
						// Bounce off the surface and reduce velocity
						float speedLossFactor = 0.5f; // lose 1/2 spd

						if (colA.collidedSides.left || colA.collidedSides.right)
						{
							// Hit left or right wall, reverse horizontal velocity and reduce
							movementCompA.velocity.x = -movementCompA.velocity.x * speedLossFactor;
						}
						if (colA.collidedSides.top || colA.collidedSides.bottom)
						{
							// Hit top or bottom, reverse vertical velocity and reduce
							movementCompA.velocity.y = -movementCompA.velocity.y * speedLossFactor;
						}
					}
					// Star vs star: lose all momentum
					else if (!removedB && isStarB)
					{
						// Lose all spd
						movementCompA.velocity.x = 0.0f;
						movementCompA.velocity.y = 0.0f;

						// Other star loses all spd
						if (entityManager.template HasComponent<MovementComponent>(idB))
						{
							auto &movementCompB = entityManager.template GetComponent<MovementComponent>(idB);
							movementCompB.velocity.x = 0.0f;
							movementCompB.velocity.y = 0.0f;
						}
					}
				}
				// Also handle case where idB is a star
				else if (!removedB && isStarB && entityManager.template HasComponent<MovementComponent>(idB))
				{
					auto &movementCompB = entityManager.template GetComponent<MovementComponent>(idB);

					// Star vs static object: bounce and lose speed
					if (colA.isStatic)
					{
						// Bounce off the surface and reduce velocity
						float speedLossFactor = 0.5f; // lose 1/2 spd

						if (colB.collidedSides.left || colB.collidedSides.right)
						{
							// Hit left or right wall, reverse horizontal velocity and reduce
							movementCompB.velocity.x = -movementCompB.velocity.x * speedLossFactor;
						}
						if (colB.collidedSides.top || colB.collidedSides.bottom)
						{
							// Hit top or bottom, reverse vertical velocity and reduce
							movementCompB.velocity.y = -movementCompB.velocity.y * speedLossFactor;
						}
					}
					// Star vs star: lose all momentum
					else if (!removedA && isStarA)
					{
						// Lose all spd
						movementCompB.velocity.x = 0.0f;
						movementCompB.velocity.y = 0.0f;

						// Other star loses all spd
						if (entityManager.template HasComponent<MovementComponent>(idA))
						{
							auto &movementCompA = entityManager.template GetComponent<MovementComponent>(idA);
							movementCompA.velocity.x = 0.0f;
							movementCompA.velocity.y = 0.0f;
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

				// Update sprite components for each digit in the score
				// Convert score to string and update display entities
				std::string scoreStr = std::to_string(scoreComp.currentScore);

				for (size_t i = 0; i < scoreComp.displayEntityIds.size() && i < scoreStr.length(); ++i)
				{
					EntityID displayEntityId = scoreComp.displayEntityIds[i];
					if (mEntityManager.template HasComponent<SpriteComponent>(displayEntityId))
					{
						int digit = scoreStr[scoreStr.length() - 1 - i] - '0';
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
		// Zone-based camera system with dead zones and tracking zones
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
							float trackingSpeed = 0.0f; // Interpolation speed (0 = instant, higher = slower)

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
								trackingSpeed = 5.0f; // Tuneable: higher = slower tracking
							}
							else if (playerXPercent > 0.7f)
							{
								// Right 10-30% zone
								targetCameraX = trackedLocation.position.x - (windowWidth * 0.7f - windowWidth * 0.5f) * cameraComp.scale;
								trackingSpeed = 5.0f; // Tuneable: higher = slower tracking
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
										// Target: playerWindowX = windowWidth * 0.5 (center)
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
									// Smooth interpolation using exponential moving average
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

		window->clear(); // Clear the screen before drawing

		for (auto it = entityManager.begin(); it != entityManager.end(); ++it)
		{
			if (!it->isActive())
				continue;
			EntityID entityId = it->getID();

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
		// Handle spawning for entities with SpawnComponent
		float deltaTime = 1.0f / 60.0f;
		static std::random_device rd;
		static std::mt19937 gen(rd());
		static std::uniform_real_distribution<float> angleDist(0.0f, 2.0f * 3.14159f);
		static std::uniform_real_distribution<float> speedDist(50.0f, 150.0f);

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
					// Get spawner location
					if (mEntityManager.template HasComponent<LocationComponent>(entityId))
					{
						auto &spawnerLocation = mEntityManager.template GetComponent<LocationComponent>(entityId);

						// Get sprite size from sprite manager for sprite rendering
						auto &spriteManager = GetSpriteManager();
						sf::Sprite &sprite = spriteManager.GetSprite(spawnComp.spriteId);
						sf::IntRect textureRect = sprite.getTextureRect();

						// Sprite bounds should be relative to entity position (0, 0), not texture coordinates
						Rect spriteBounds(
							0.0f,
							0.0f,
							static_cast<float>(textureRect.size.x),
							static_cast<float>(textureRect.size.y));

						// Create AABB bounds using tileW and tileH from SpawnComponent
						Rect collisionBounds = {0.0f, 0.0f, spawnComp.tileW, spawnComp.tileH};

						// Create new star entity
						EntityID starId = mEntityManager.CreateEntity("star");

						// Add LocationComponent
						LocationComponent starLocation;
						starLocation.position = spawnerLocation.position;
						mEntityManager.template AddComponent<LocationComponent>(starId, starLocation);

						// Add MovementComponent with random velocity
						MovementComponent starMovement;
						float angle = angleDist(gen);
						float speed = speedDist(gen);
						starMovement.velocity = Point2D(std::cos(angle) * speed, std::sin(angle) * speed);
						mEntityManager.template AddComponent<MovementComponent>(starId, starMovement);

						// Add GravityComponent
						GravityComponent starGravity;
						starGravity.acceleration = Point2D(0, 600.0f); // Gravity acceleration
						mEntityManager.template AddComponent<GravityComponent>(starId, starGravity);

						// Add CollisionComponent with AABB bounds from tileW and tileH
						CollisionComponent starCollision;
						starCollision.isStatic = false;
						starCollision.currentBounds = collisionBounds;
						starCollision.previousBounds = collisionBounds;
						mEntityManager.template AddComponent<CollisionComponent>(starId, starCollision);

						// Add SpriteComponent with spriteId and sprite bounds
						SpriteComponent starSprite;
						starSprite.spriteId = spawnComp.spriteId;
						starSprite.bounds = spriteBounds;
						starSprite.inWorldSpace = true;
						mEntityManager.template AddComponent<SpriteComponent>(starId, starSprite);

						// Reset timer and decrement spawn events
						spawnComp.timeToNextSpawn = spawnComp.timeBetweenSpawns;
						if (spawnComp.totalSpawnEvents > 0)
							spawnComp.totalSpawnEvents--;
					}
				}
			}
		}
	}

	// inline void ResolveAABBCollision(Rect &a, const Rect &b, CollisionFlags &flagsA, CollisionFlags &flagsB)
	// {
	// 	Point2D overlap = GetOverlap(a, b);

	// 	if (overlap.x <= 0.0f || overlap.y <= 0.0f)
	// 		return; // no real overlap

	// 	// Resolve Y-axis
	// 	if (a.topLeft.y < b.topLeft.y)
	// 	{
	// 		a.topLeft.y -= overlap.y;
	// 		flagsA.bottom = true;
	// 		flagsB.top = true;
	// 	}
	// 	else
	// 	{
	// 		a.topLeft.y += overlap.y;
	// 		flagsA.top = true;
	// 		flagsB.bottom = true;
	// 	}

	// 	// Resolve X-axis
	// 	if (a.topLeft.x < b.topLeft.x)
	// 	{
	// 		a.topLeft.x -= overlap.x;
	// 		flagsA.right = true;
	// 		flagsB.left = true;
	// 	}
	// 	else
	// 	{
	// 		a.topLeft.x += overlap.x;
	// 		flagsA.left = true;
	// 		flagsB.right = true;
	// 	}
	// }


	// inline void ResolveAABBCollision(Rect &a, const Rect &b,
    //                              CollisionFlags &flagsA, CollisionFlags &flagsB)
	// {
	// 	Point2D overlap = GetOverlap(a, b);
	// 	if (overlap.x <= 0.0f || overlap.y <= 0.0f)
	// 		return;

	// 	// PRIORITIZE VERTICAL COLLISIONS (GROUND/CEILING)
	// 	if (overlap.y <= overlap.x)
	// 	{
	// 		// --- Y-AXIS RESOLUTION ---
	// 		if (a.topLeft.y < b.topLeft.y)
	// 		{
	// 			// A landed on top of B
	// 			a.topLeft.y -= overlap.y;
	// 			flagsA.bottom = true;
	// 			flagsB.top = true;
	// 		}
	// 		else
	// 		{
	// 			// A hit B from below
	// 			a.topLeft.y += overlap.y;
	// 			flagsA.top = true;
	// 			flagsB.bottom = true;
	// 		}
	// 	}
	// 	else
	// 	{
	// 		// --- X-AXIS RESOLUTION ---
	// 		if (a.topLeft.x < b.topLeft.x)
	// 		{
	// 			a.topLeft.x -= overlap.x;
	// 			flagsA.right = true;
	// 			flagsB.left = true;
	// 		}
	// 		else
	// 		{
	// 			a.topLeft.x += overlap.x;
	// 			flagsA.left = true;
	// 			flagsB.right = true;
	// 		}
	// 	}
	// }
// inline void ResolveAABBCollision(Rect &a, const Rect &b,
//                                  CollisionFlags &flagsA, CollisionFlags &flagsB)
// {
//     Point2D overlap = GetOverlap(a, b);
//     if (overlap.x <= 0.0f || overlap.y <= 0.0f)
//         return;

//     // PRIORITIZE VERTICAL COLLISIONS
//     if (overlap.y <= overlap.x)
//     {
//         // ---------- Y AXIS COLLISION ----------
//         if (a.topLeft.y < b.topLeft.y)
//         {
//             // A landed on B → SNAP A ON TOP OF B
//             a.topLeft.y = b.topLeft.y - a.height;
//             flagsA.bottom = true;
//             flagsB.top = true;
//         }
//         else
//         {
//             // A hit B from below
//             a.topLeft.y = b.topLeft.y + b.height;
//             flagsA.top = true;
//             flagsB.bottom = true;
//         }
//     }
//     else
//     {
//         // ---------- X AXIS COLLISION ----------
//         if (a.topLeft.x < b.topLeft.x)
//         {
//             a.topLeft.x = b.topLeft.x - a.width;
//             flagsA.right = true;
//             flagsB.left = true;
//         }
//         else
//         {
//             a.topLeft.x = b.topLeft.x + b.width;
//             flagsA.left = true;
//             flagsB.right = true;
//         }
//     }
// }

	inline void ResolveAABBCollision(Rect &a, const Rect &b,
                                 const Rect &aPrev, const Rect &bPrev,
                                 CollisionFlags &flagsA, CollisionFlags &flagsB)
{
    Point2D overlap = GetOverlap(a, b);
    if (overlap.x <= 0.0f || overlap.y <= 0.0f)
        return;

    Point2D prevCenterA = aPrev.topLeft + Point2D(aPrev.width * 0.5f, aPrev.height * 0.5f);
    Point2D prevCenterB = bPrev.topLeft + Point2D(bPrev.width * 0.5f, bPrev.height * 0.5f);

    float dx = prevCenterA.x - prevCenterB.x;
    float dy = prevCenterA.y - prevCenterB.y;

    if (std::abs(dy) > std::abs(dx))
    {
        // Resolve Y axis first
        if (dy > 0)
        {
            // Came from below — hit ceiling
            a.topLeft.y += overlap.y;
            flagsA.top = true;
            flagsB.bottom = true;
        }
        else
        {
			// Came from above — landed on top
            a.topLeft.y -= overlap.y;
            flagsA.bottom = true;
            flagsB.top = true;
        }
    }
    else
    {
        // Resolve X axis
        if (dx < 0)
        {
            // Came from left
            a.topLeft.x -= overlap.x;
            flagsA.right = true;
            flagsB.left = true;
        }
        else
        {
            // Came from right
            a.topLeft.x += overlap.x;
            flagsA.left = true;
            flagsB.right = true;
        }
    }
}



}