#pragma once

#include "SystemManager.h"
#include "Scene.h"
#include "InputComponent.h"
#include "LocationComponent.h"
#include "CollisionComponent.h"
#include "AnimationComponent.h"
#include "SpellComponent.h"
#include "ProjectileComponent.h"
#include "SpriteComponent.h"
#include "SoundManager.h"
#include <unordered_map>
#include <cmath>
#include <random>

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
			static std::unordered_map<EntityID, float> walkSoundTimer;
			static std::unordered_map<EntityID, bool> wasCastPressed;
			static std::unordered_map<EntityID, float> castAnimTimer;

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

						// Play wall push sound when hitting a wall
						if ((onWallLeft || onWallRight) && 
							((onWallLeft && (movingLeft || movementComp.velocity.x < -10.0f)) ||
							 (onWallRight && (movingRight || movementComp.velocity.x > 10.0f)) ||
							 (wasFalling && !onGround)))
						{
							if (!wasPushingWall[entityId])
								soundManager.PlaySound("wall_push");
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

					// Play walking sound when moving on ground
					const float walkSoundInterval = 0.35f; // Time between footstep sounds
					bool isWalking = onGround && (movingLeft || movingRight) && 
									 std::abs(movementComp.velocity.x) > 50.0f;
					
					if (isWalking)
					{
						walkSoundTimer[entityId] -= deltaTime;
						if (walkSoundTimer[entityId] <= 0.0f)
						{
							soundManager.PlaySound("walk");
							walkSoundTimer[entityId] = walkSoundInterval;
						}
					}
					else
					{
						// Reset timer when not walking so sound plays immediately when starting to walk
						walkSoundTimer[entityId] = 0.0f;
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

					bool justJumped = false;
					if (jumpPressed && !wasJumpPressedLastFrame)
					{
						if (onGround)
						{
							movementComp.velocity.y = jumpVelocity;
							soundManager.PlaySound("jump");
							recentlyJumped[entityId] = true;
							justJumped = true;
							
							// Trigger jump animation immediately
							if (entityManager.template HasComponent<AnimationComponent>(entityId))
							{
								auto &anim = entityManager.template GetComponent<AnimationComponent>(entityId);
								if (anim.animations.count("jump") > 0)
								{
									anim.Play("jump", false); // Don't loop jump animation
									// Set sprite immediately to first frame
									if (entityManager.template HasComponent<SpriteComponent>(entityId))
									{
										auto &sprite = entityManager.template GetComponent<SpriteComponent>(entityId);
										sprite.spriteId = anim.animations["jump"][0];
									}
								}
							}
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
							justJumped = true;
							
							// Trigger jump animation for wall jump too
							if (entityManager.template HasComponent<AnimationComponent>(entityId))
							{
								auto &anim = entityManager.template GetComponent<AnimationComponent>(entityId);
								if (anim.animations.count("jump") > 0)
								{
									anim.Play("jump", false);
									if (entityManager.template HasComponent<SpriteComponent>(entityId))
									{
										auto &sprite = entityManager.template GetComponent<SpriteComponent>(entityId);
										sprite.spriteId = anim.animations["jump"][0];
									}
								}
							}
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

					// Handle spell casting (E key)
					sf::Keyboard::Scancode scancodeE = sf::Keyboard::delocalize(sf::Keyboard::Key::E);
					bool castPressed = (scancodeE != sf::Keyboard::Scan::Unknown &&
										static_cast<size_t>(scancodeE) < sf::Keyboard::ScancodeCount)
										   ? inputComp.keydown[static_cast<size_t>(scancodeE)]
										   : false;

					bool wasCastPressedLastFrame = wasCastPressed[entityId];
					wasCastPressed[entityId] = castPressed;

					// Update cast animation timer
					if (castAnimTimer[entityId] > 0.0f)
					{
						castAnimTimer[entityId] -= deltaTime;
					}

					// Cast spell on E key press
					bool justCast = false;
					if (castPressed && !wasCastPressedLastFrame)
					{
						justCast = true;
						// Play magic animation immediately
						if (entityManager.template HasComponent<AnimationComponent>(entityId))
						{
							auto &anim = entityManager.template GetComponent<AnimationComponent>(entityId);
							if (anim.animations.count("magic") > 0)
							{
								anim.Play("magic", false); // Don't loop magic animation
								castAnimTimer[entityId] = anim.frameDuration * anim.animations["magic"].size();
								// Set sprite immediately to first frame
								if (entityManager.template HasComponent<SpriteComponent>(entityId))
								{
									auto &sprite = entityManager.template GetComponent<SpriteComponent>(entityId);
									sprite.spriteId = anim.animations["magic"][0];
								}
							}
						}

						// Spawn fireball
						if (entityManager.template HasComponent<LocationComponent>(entityId))
						{
							auto &playerLoc = entityManager.template GetComponent<LocationComponent>(entityId);
							
							// Determine facing direction based on movement or last direction
							int facingDir = 1; // Default right
							if (movingLeft && !movingRight)
								facingDir = -1;
							else if (movingRight && !movingLeft)
								facingDir = 1;
							
							// Store facing direction for entities with SpellComponent
							if (entityManager.template HasComponent<SpellComponent>(entityId))
							{
								auto &spellComp = entityManager.template GetComponent<SpellComponent>(entityId);
								if (movingLeft && !movingRight)
									spellComp.facingDirection = -1;
								else if (movingRight && !movingLeft)
									spellComp.facingDirection = 1;
								facingDir = spellComp.facingDirection;
								
								// Check cooldown
								if (spellComp.castCooldown <= 0.0f)
								{
									const auto &props = spellComp.spellProperties[static_cast<size_t>(SpellType::Fire)];
									
									// Calculate spawn position - spawn far enough to avoid colliding with player
									// Player is 32x32, fireball collision is 28x28, need at least 32 offset
									float spawnOffsetX = facingDir * 40.0f;
									Point2D spawnPos = {
										playerLoc.position.x + spawnOffsetX,
										playerLoc.position.y
									};

									// Create fireball entity
									EntityID fireballId = entityManager.CreateEntity("fireball");

									entityManager.template AddComponent<LocationComponent>(fireballId, LocationComponent(spawnPos));

									MovementComponent fireballMovement;
									fireballMovement.velocity = Point2D(facingDir * props.speed, 0.0f);
									entityManager.template AddComponent<MovementComponent>(fireballId, fireballMovement);

									ProjectileComponent projectile;
									projectile.spellType = SpellType::Fire;
									projectile.damage = props.damage;
									projectile.lifetime = props.lifetime;
									projectile.maxLifetime = props.lifetime;
									projectile.ownerEntityId = entityId;
									projectile.active = true;
									projectile.explosionFrames = props.explosionFrames;
									projectile.explosionSize = props.explosionSize;
									entityManager.template AddComponent<ProjectileComponent>(fireballId, projectile);

									Rect collisionBounds(0.0f, 0.0f, props.size, props.size);
									CollisionComponent collision(collisionBounds, false);
									entityManager.template AddComponent<CollisionComponent>(fireballId, collision);

									// Create fireball sprite
									SpriteComponent fireballSprite;
									fireballSprite.spriteId = props.spriteId;
									fireballSprite.bounds = Rect(0.0f, 0.0f, 28.0f, 22.0f); // Actual fireball size
									fireballSprite.inWorldSpace = true;
									entityManager.template AddComponent<SpriteComponent>(fireballId, fireballSprite);

									// Start cooldown
									spellComp.castCooldown = props.cooldown;

									// Play fire cast sound
									static std::mt19937 rng(std::random_device{}());
									std::uniform_int_distribution<int> dist(1, 2);
									std::string soundName = "fire_cast_" + std::to_string(dist(rng));
									soundManager.PlaySound(soundName);
								}
							}
						}
					}

					// Handle animation transitions back to idle/run
					// Skip if we just triggered an action this frame
					if (!justJumped && !justCast && entityManager.template HasComponent<AnimationComponent>(entityId))
					{
						auto &anim = entityManager.template GetComponent<AnimationComponent>(entityId);
						
						// If casting animation is done
						bool castingDone = castAnimTimer[entityId] <= 0.0f;
						bool isCasting = anim.currentAnimation == "magic" && !castingDone;
						// Consider jumping if in air OR if we recently jumped (to catch first frame)
						bool isJumping = anim.currentAnimation == "jump" && (!onGround || recentlyJumped[entityId]);
						
						// Don't interrupt casting or jumping animations
						if (!isCasting && !isJumping)
						{
							// Transition to appropriate animation based on state
							if (!onGround)
							{
								// In air - show jump animation
								if (anim.currentAnimation != "jump" && anim.animations.count("jump") > 0)
								{
									anim.Play("jump", false);
								}
							}
							else if (movingLeft || movingRight)
							{
								// Running on ground
								if (anim.currentAnimation != "run" && anim.animations.count("run") > 0)
								{
									anim.Play("run", true);
								}
							}
							else
							{
								// Idle on ground
								if (anim.currentAnimation != "idleA" && anim.animations.count("idleA") > 0)
								{
									anim.Play("idleA", true);
								}
							}
						}
					}
				}
			}

			return true;
		}
	};

} // namespace ECSEngine
