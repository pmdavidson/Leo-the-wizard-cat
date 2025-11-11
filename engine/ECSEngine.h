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

namespace ECSEngine
{

	template <typename... Components>
	class ECSEngine
	{
	public:
		ECSEngine(unsigned int width, unsigned int height, const std::string &name);

		void Run();

		SoundManager &GetSoundManager()
		{
			return mSoundManager;
		}

		SpriteManager &GetSpriteManager()
		{
			return mSpriteManager;
		}

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
		sf::RenderWindow *window = mWindowManager.GetWindow(); //add GetWindow at the top?
		
		// Main game loop
		while (window->isOpen())
		{
			ProcessEvents();
			CollisionSystemUpdate();
			InputSystem();
			GravitySystem();
			MovementSystem();
			SpawnSystem();
			ScoreSystem();
			CameraSystem();
			
			// Clear and draw sprites
			window->clear();
			
			// Draw all sprites (SpriteSystem)
			for (auto it = mEntityManager.begin(); it != mEntityManager.end(); ++it)
			{
				if (!it->valid) continue;
				EntityID entityId = it->id;
				
				if (mEntityManager.template HasComponent<SpriteComponent>(entityId))
				{
					auto &spriteComp = mEntityManager.template GetComponent<SpriteComponent>(entityId);
					
					if (mEntityManager.template HasComponent<LocationComponent>(entityId))
					{
						auto &locationComp = mEntityManager.template GetComponent<LocationComponent>(entityId);
						
						// Get the sprite
						sf::Sprite &sprite = mSpriteManager.GetSprite(spriteComp.spriteId);
						
						// Calculate position
						Point2D drawPos;
						if (spriteComp.inWorldSpace)
						{
							// Convert world to window coordinates
							drawPos = mWindowManager.WorldToWindow(
								Point2D(locationComp.position.x + spriteComp.bounds.topLeft.x,
									locationComp.position.y + spriteComp.bounds.topLeft.y));
								}
								else
								{
									// Screen space - use bounds directly
									drawPos = spriteComp.bounds.topLeft;
								}
								
								sprite.setPosition(sf::Vector2f(drawPos.x, drawPos.y));
								window->draw(sprite);
							}
						}
					}
					
					window->display();
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
		
						// Update input components for all entities with InputComponent
						for (auto it = mEntityManager.begin(); it != mEntityManager.end(); ++it)
						{
							EntityID entityId = std::distance(mEntityManager.begin(), it);
		
							if (mEntityManager.template HasComponent<InputComponent>(entityId))
							{
								auto &inputComp = mEntityManager.template GetComponent<InputComponent>(entityId);
								inputComp.keydown.set(static_cast<size_t>(scancode));
							}
						}
					}
					else if (auto keyEvent = event->getIf<sf::Event::KeyReleased>())
					{
						// Convert Key to Scancode for InputComponent
						sf::Keyboard::Scancode scancode = sf::Keyboard::delocalize(keyEvent->code);
		
						// Clear key when released
						for (auto it = mEntityManager.begin(); it != mEntityManager.end(); ++it)
						{
							EntityID entityId = std::distance(mEntityManager.begin(), it);
		
							if (mEntityManager.template HasComponent<InputComponent>(entityId))
							{
								auto &inputComp = mEntityManager.template GetComponent<InputComponent>(entityId);
								inputComp.keydown.reset(static_cast<size_t>(scancode));
							}
						}
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
			EntityID entityId = std::distance(mEntityManager.begin(), it);

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
				bool movingLeft = inputComp.keydown[static_cast<size_t>(sf::Keyboard::delocalize(sf::Keyboard::Key::A))];
				bool movingRight = inputComp.keydown[static_cast<size_t>(sf::Keyboard::delocalize(sf::Keyboard::Key::D))];

				// Check collision sides for wall jumping
				bool onGround = false;
				bool onWallLeft = false;
				bool onWallRight = false;
				bool wasFalling = false;

				if (mEntityManager.template HasComponent<CollisionComponent>(entityId))
				{
					auto &collisionComp = mEntityManager.template GetComponent<CollisionComponent>(entityId);
					onGround = collisionComp.collisionSides[static_cast<size_t>(CollisionSide::Bottom)];
					onWallLeft = collisionComp.collisionSides[static_cast<size_t>(CollisionSide::Left)];
					onWallRight = collisionComp.collisionSides[static_cast<size_t>(CollisionSide::Right)];
					wasFalling = movementComp.velocity.y > 0;
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
				bool jumpPressed = inputComp.keydown[static_cast<size_t>(sf::Keyboard::delocalize(sf::Keyboard::Key::W))] ||
								   inputComp.keydown[static_cast<size_t>(sf::Keyboard::delocalize(sf::Keyboard::Key::Space))];

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
					}
				}

				// Handle fast fall (S key while falling)
				bool downPressed = inputComp.keydown[static_cast<size_t>(sf::Keyboard::delocalize(sf::Keyboard::Key::S))];
				if (downPressed && !onGround && movementComp.velocity.y > 0)
				{
					// Increase fall speed
					movementComp.velocity.y += 200.0f * deltaTime; // Additional downward acceleration
				}
			}
		}
	}

	template <typename... Components>
	void ECSEngine<Components...>::MovementSystem() //update with GravitySystem now
	{
		float deltaTime = 1.0f / 60.0f; //assuming 60FPS?

		// Update locations based on velocity and apply gravity
		for (auto it = mEntityManager.begin(); it != mEntityManager.end(); ++it)
		{
			EntityID entityId = std::distance(mEntityManager.begin(), it);

			if (mEntityManager.template HasComponent<LocationComponent>(entityId) &&
				mEntityManager.template HasComponent<MovementComponent>(entityId))
			{
				auto &locationComp = mEntityManager.template GetComponent<LocationComponent>(entityId);
				auto &movementComp = mEntityManager.template GetComponent<MovementComponent>(entityId);

				// Apply gravity if entity has GravityComponent
				if (mEntityManager.template HasComponent<GravityComponent>(entityId))
				{
					auto &gravityComp = mEntityManager.template GetComponent<GravityComponent>(entityId);
					movementComp.velocity += gravityComp.acceleration * deltaTime;
				}

				// Check for wall push while falling (for wall jump sound)
				if (mEntityManager.template HasComponent<InputComponent>(entityId) &&
					mEntityManager.template HasComponent<CollisionComponent>(entityId))
				{
					auto &collisionComp = mEntityManager.template GetComponent<CollisionComponent>(entityId);
					bool onWall = collisionComp.collisionSides[static_cast<size_t>(CollisionSide::Left)] ||
								  collisionComp.collisionSides[static_cast<size_t>(CollisionSide::Right)];
					bool falling = movementComp.velocity.y > 0;

					// Wall push sound is handled in InputSystem when wall jumping
				}

				// Update position based on velocity
				locationComp.position.x += movementComp.velocity.x * deltaTime;
				locationComp.position.y += movementComp.velocity.y * deltaTime;
			}
		}
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
			EntityID entityId = std::distance(mEntityManager.begin(), it);

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

						// Create new star entity
						EntityID starId = mEntityManager.CreateEntity("star");

						// Add LocationComponent
						LocationComponent starLocation;
						starLocation.position = spawnerLocation.position;
						mEntityManager.template AddComponent(starId, starLocation);

						// Add MovementComponent with random velocity
						MovementComponent starMovement;
						float angle = angleDist(gen);
						float speed = speedDist(gen);
						starMovement.velocity = Point2D(std::cos(angle) * speed, std::sin(angle) * speed);
						mEntityManager.template AddComponent(starId, starMovement);

						// Add GravityComponent
						GravityComponent starGravity;
						starGravity.acceleration = Point2D(0, 600.0f); // Gravity acceleration
						mEntityManager.template AddComponent(starId, starGravity);

						// Add CollisionComponent (dynamic)
						CollisionComponent starCollision;
						starCollision.isStatic = false;
						mEntityManager.template AddComponent(starId, starCollision);

						SpriteComponent starSprite;
						starSprite.inWorldSpace = true;
						mEntityManager.template AddComponent(starId, starSprite);

						// Reset timer and decrement spawn events
						spawnComp.timeToNextSpawn = spawnComp.timeBetweenSpawns;
						if (spawnComp.totalSpawnEvents > 0)
							spawnComp.totalSpawnEvents--;
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
			EntityID entityId = std::distance(mEntityManager.begin(), it);

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
		float deltaTime = 1.0f / 60.0f;

		// Update camera position based on CameraFollower
		for (auto it = mEntityManager.begin(); it != mEntityManager.end(); ++it)
		{
			EntityID entityId = std::distance(mEntityManager.begin(), it);

			if (mEntityManager.template HasComponent<CameraFollower>(entityId))
			{
				auto &cameraFollower = mEntityManager.template GetComponent<CameraFollower>(entityId);
				EntityID trackedEntityId = cameraFollower.entityId;

				// Find camera entity (entity with CameraComponent)
				for (auto camIt = mEntityManager.begin(); camIt != mEntityManager.end(); ++camIt)
				{
					EntityID camEntityId = std::distance(mEntityManager.begin(), camIt);

					if (mEntityManager.template HasComponent<CameraComponent>(camEntityId))
					{
						auto &cameraComp = mEntityManager.template GetComponent<CameraComponent>(camEntityId);

						// Get tracked entity's location
						if (mEntityManager.template HasComponent<LocationComponent>(trackedEntityId))
						{
							auto &trackedLocation = mEntityManager.template GetComponent<LocationComponent>(trackedEntityId);

							// Get window dimensions
							sf::RenderWindow *window = mWindowManager.GetWindow();
							unsigned int windowWidth = window->getSize().x;
							unsigned int windowHeight = window->getSize().y;

							// Convert player world position to window position
							Point2D playerWindowPos = mWindowManager.WorldToWindow(trackedLocation.position);

							// Calculate zones (10%, 30%, 40% center)
							float left10 = windowWidth * 0.1f;
							float left30 = windowWidth * 0.3f;
							float right30 = windowWidth * 0.7f;
							float right10 = windowWidth * 0.9f;
							float center40Left = windowWidth * 0.3f;
							float center40Right = windowWidth * 0.7f;

							// Camera follows player in X, fixed Y
							float targetCameraX = cameraComp.position.x;
							float targetCameraY = cameraComp.position.y; // Fixed Y

							// Check which zone player is in
							if (playerWindowPos.x < left10 || playerWindowPos.x > right10)
							{
								// hardzone
								targetCameraX = trackedLocation.position.x;
							}
						else if (playerWindowPos.x < left30 || playerWindowPos.x > right30)
						{
							float tweenSpeed = 5.0f;
							float desiredX = trackedLocation.position.x;
							float diff = desiredX - cameraComp.position.x;
							targetCameraX = cameraComp.position.x + diff * tweenSpeed * deltaTime;
						}
						else
						{
							float desiredX = trackedLocation.position.x;
							float diff = desiredX - cameraComp.position.x;
							float tweenSpeed = 3.0f;
							targetCameraX = cameraComp.position.x + diff * tweenSpeed * deltaTime;
						}

							cameraComp.position.x = targetCameraX;

							// Apply camera shake if present
							if (mEntityManager.template HasComponent<CameraShake>(camEntityId))
							{
								auto &shakeComp = mEntityManager.template GetComponent<CameraShake>(camEntityId);

								if (shakeComp.framesRemaining > 0)
								{
									// Apply random shake offset
									static std::random_device rd;
									static std::mt19937 gen(rd());
									std::uniform_real_distribution<float> shakeDist(-1.0f, 1.0f);

									float shakeX = shakeComp.magnitude.x * shakeDist(gen);
									float shakeY = shakeComp.magnitude.y * shakeDist(gen);

									cameraComp.position.x += shakeX;
									cameraComp.position.y += shakeY;

									shakeComp.framesRemaining--;

									// Remove shake component when done
									if (shakeComp.framesRemaining <= 0)
									{
										// Component will be removed in next frame
									}
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
		auto& entityManager = GetEntityManager();
		auto& spriteManager = GetSpriteManager();
		auto& windowManager = GetWindowManager();
		sf::RenderWindow* window = windowManager.GetWindow(); //add getWindow at top?

		window->clear(); // Clear the screen before drawing

		for (auto it = entityManager.begin(); it != entityManager.end(); ++it)
		{
			if (!it->valid) continue;
			EntityID entityId = it->id;

			if (entityManager.template HasComponent<SpriteComponent>(entityId))
			{
				auto& spriteComp = entityManager.template GetComponent<SpriteComponent>(entityId);

				sf::Sprite& sprite = spriteManager.GetSprite(spriteComp.spriteId);
				Point2D drawPos;

				if (spriteComp.inWorldSpace && entityManager.template HasComponent<LocationComponent>(entityId))
				{
					auto& locationComp = entityManager.template GetComponent<LocationComponent>(entityId);
					Point2D worldPos = {
						locationComp.position.x + spriteComp.bounds.topLeft.x,
						locationComp.position.y + spriteComp.bounds.topLeft.y
					};
					drawPos = windowManager.WorldToWindow(worldPos);
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
	void ECSEngine<Components...>::GravitySystem()
	{
		float deltaTime = 1.0f / 60.0f; //assuming 60FPS?

		for (auto it = mEntityManager.begin(); it != mEntityManager.end(); ++it)
		{
			if (!it->valid) continue;
			EntityID entityId = it->id;

			if (mEntityManager.template HasComponent<GravityComponent>(entityId) &&
				mEntityManager.template HasComponent<MovementComponent>(entityId))
			{
				auto& gravityComp = mEntityManager.template GetComponent<GravityComponent>(entityId);
				auto& movementComp = mEntityManager.template GetComponent<MovementComponent>(entityId);

				movementComp.velocity += gravityComp.acceleration * deltaTime;
			}
		}
	}

	template <typename... Components>
	void ECSEngine<Components...>::CollisionSystemUpdate()
	{
		for (auto it = mEntityManager.begin(); it != mEntityManager.end(); ++it)
		{
			if (!it->valid) continue;
			EntityID entityId = it->id;

			if (mEntityManager.template HasComponent<CollisionComponent>(entityId))
			{
				auto& collisionComp = mEntityManager.template GetComponent<CollisionComponent>(entityId);
				collisionComp.previousBounds = collisionComp.currentBounds;
			}
		}
	}

	template <typename... Components>
	void ECSEngine<Components...>::CollisionSystem()
	{
		auto& entityManager = GetEntityManager();

		for (auto itA = entityManager.begin(); itA != entityManager.end(); ++itA)
		{
			if (!itA->valid) continue;
			EntityID idA = itA->id;

			// A must have collision and must be dynamic
			if (!entityManager.template HasComponent<CollisionComponent>(idA)) continue;

			auto& colA = entityManager.template GetComponent<CollisionComponent>(idA);
			if (colA.isStatic) continue;

			// Reset collision flags
			colA.collidedSides = {};

			for (auto itB = entityManager.begin(); itB != entityManager.end(); ++itB) //change to start from itA? and mathutil intersect?
			{
				if (!itB->valid) continue;
				EntityID idB = itB->id;

				if (idA == idB) continue; // skip self
				if (!entityManager.template HasComponent<CollisionComponent>(idB)) continue;

				auto& colB = entityManager.template GetComponent<CollisionComponent>(idB);
				const Rect& a = colA.currentBounds;
				const Rect& b = colB.currentBounds;

				// AABB check
				bool intersecting =
					a.topLeft.x < b.topLeft.x + b.size.x &&
					a.topLeft.x + a.size.x > b.topLeft.x &&
					a.topLeft.y < b.topLeft.y + b.size.y &&
					a.topLeft.y + a.size.y > b.topLeft.y;

				if (!intersecting) continue;

				// Compute overlap on each axis
				float overlapX1 = b.topLeft.x + b.size.x - a.topLeft.x;
				float overlapX2 = a.topLeft.x + a.size.x - b.topLeft.x;
				float overlapX = std::min(overlapX1, overlapX2);

				float overlapY1 = b.topLeft.y + b.size.y - a.topLeft.y;
				float overlapY2 = a.topLeft.y + a.size.y - b.topLeft.y;
				float overlapY = std::min(overlapY1, overlapY2);

				// Resolve along the smaller overlap axis
				if (overlapX < overlapY)
				{
					if (a.topLeft.x < b.topLeft.x)
					{
						colA.currentBounds.topLeft.x -= overlapX;
						colA.collidedSides.right = true;
					}
					else
					{
						colA.currentBounds.topLeft.x += overlapX;
						colA.collidedSides.left = true;
					}
				}
				else
				{
					if (a.topLeft.y < b.topLeft.y)
					{
						colA.currentBounds.topLeft.y -= overlapY;
						colA.collidedSides.bottom = true;
					}
					else
					{
						colA.currentBounds.topLeft.y += overlapY;
						colA.collidedSides.top = true;
					}
				}
			}
		}
	}


}
