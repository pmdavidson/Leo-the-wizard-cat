#pragma once

#include "SystemManager.h"
#include "Scene.h"
#include "InputComponent.h"
#include "LocationComponent.h"
#include "CollisionComponent.h"
#include "SoundManager.h"
#include <unordered_map>

namespace ECSEngine
{

	template <typename... Components>
	class InputSystem : public System<Components...>
	{
	public:
		bool Run(Scene<Components...> &scene) override
		{
			auto &entityManager = scene.GetEntityManager();
			auto &soundManager = scene.GetSoundManager();
			float deltaTime = 1.0f / 60.0f;

			// Track if we jumped recently for landing detection
			static std::unordered_map<EntityID, bool> recentlyJumped;
			static std::unordered_map<EntityID, bool> wasPushingWall;
			static std::unordered_map<EntityID, bool> wasJumpPressed;

			for (auto it = entityManager.begin(); it != entityManager.end(); ++it)
			{
				if (!it->isActive())
					continue;
				EntityID entityId = it->getID();

				if (entityManager.template HasComponent<InputComponent>(entityId) &&
					entityManager.template HasComponent<MovementComponent>(entityId))
				{
					auto &inputComp = entityManager.template GetComponent<InputComponent>(entityId);
					auto &movementComp = entityManager.template GetComponent<MovementComponent>(entityId);

					// Platformer movement constants
					const float maxSpeed = 200.0f;
					const float acceleration = 800.0f;
					const float deceleration = 1000.0f;
					const float jumpVelocity = -400.0f;
					const float wallJumpVelocityX = 300.0f;
					const float wallJumpVelocityY = -350.0f;

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

					if (entityManager.template HasComponent<CollisionComponent>(entityId))
					{
						auto &collisionComp = entityManager.template GetComponent<CollisionComponent>(entityId);
						onGround = collisionComp.collidedSides.bottom;
						onWallLeft = collisionComp.collidedSides.left;
						onWallRight = collisionComp.collidedSides.right;
						wasFalling = movementComp.velocity.y > 0;

						// Detect landing, if we're on ground and we recently jumped, play landing sound
						if (onGround && recentlyJumped[entityId])
						{
							soundManager.PlaySound("land");
							recentlyJumped[entityId] = false;
						}

						// Play wall push sound when pushing against wall while falling
						if ((onWallLeft || onWallRight) && wasFalling && !onGround)
						{
							bool wasPushingWallLastFrame = wasPushingWall[entityId];
							if (!wasPushingWallLastFrame)
							{
								soundManager.PlaySound("wall_push");
							}
							wasPushingWall[entityId] = true;
						}
						// wall push sound but this also registers stars so not good
						// else if ((onWallLeft || onWallRight) && !wasFalling && onGround)
						// {
						// 	bool wasPushingWallLastFrame = wasPushingWall[entityId];
						// 	if (!wasPushingWallLastFrame)
						// 	{
						// 		soundManager.PlaySound("wall_push");
						// 	}
						// 	wasPushingWall[entityId] = true;
						// }
						else
						{
							wasPushingWall[entityId] = false;
						}
					}

					// Handle horizontal movement with acceleration
					if (movingLeft && !movingRight)
					{
						movementComp.velocity.x -= acceleration * deltaTime;
						if (movementComp.velocity.x < -maxSpeed)
							movementComp.velocity.x = -maxSpeed;
					}
					else if (movingRight && !movingLeft)
					{
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

					bool wasJumpPressedLastFrame = wasJumpPressed[entityId];
					wasJumpPressed[entityId] = jumpPressed;

					if (jumpPressed && !wasJumpPressedLastFrame)
					{
						if (onGround)
						{
							movementComp.velocity.y = jumpVelocity;
							soundManager.PlaySound("jump");
							recentlyJumped[entityId] = true;
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
							soundManager.PlaySound("wall_push");
							recentlyJumped[entityId] = true;
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
						movementComp.velocity.y += 200.0f * deltaTime;
					}
				}
			}

			return true;
		}
	};

} // namespace ECSEngine
