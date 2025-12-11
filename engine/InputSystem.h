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
#include "HpComponent.h"
#include "SoundManager.h"
#include <unordered_map>
#include <cmath>
#include <random>
#include <algorithm>

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
			// Track last input direction (A = -1, D = 1, default = 1 for right)
			static std::unordered_map<EntityID, int> lastInputDirection;

			for (auto it = entityManager.begin(); it != entityManager.end(); ++it)
			{
				if (!it->isActive())
					continue;
				EntityID entityId = it->getID();

				if (entityManager.template HasComponent<InputComponent>(entityId) &&
					entityManager.template HasComponent<MovementComponent>(entityId))
				{
					// Skip input processing if player is dead or dying
					if (entityManager.template HasComponent<HpComponent>(entityId))
					{
						auto &hpComp = entityManager.template GetComponent<HpComponent>(entityId);
						if (!hpComp.isAlive || hpComp.isDying)
						{
							continue; // Skip all input processing during death
						}
					}

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
						// Update last input direction when A is pressed
						if (movingLeft)
							lastInputDirection[entityId] = -1;
					}

					bool movingRight = false;
					if (scancodeD != sf::Keyboard::Scan::Unknown && static_cast<size_t>(scancodeD) < sf::Keyboard::ScancodeCount)
					{
						movingRight = inputComp.keydown[static_cast<size_t>(scancodeD)];
						// Update last input direction when D is pressed
						if (movingRight)
							lastInputDirection[entityId] = 1;
					}
					
					// Initialize last input direction if not set (default to right)
					if (lastInputDirection.find(entityId) == lastInputDirection.end())
					{
						lastInputDirection[entityId] = 1;
					}
					
					// Update sprite flip immediately when A or D is pressed (for all entities)
					if (entityManager.template HasComponent<SpriteComponent>(entityId))
					{
						auto &spriteComp = entityManager.template GetComponent<SpriteComponent>(entityId);
						// Flip based on last input direction (A = flip, D = no flip)
						spriteComp.flipX = (lastInputDirection[entityId] < 0);
					}
					
					// Update facing direction for entities with SpellComponent
					if (entityManager.template HasComponent<SpellComponent>(entityId))
					{
						auto &spellComp = entityManager.template GetComponent<SpellComponent>(entityId);
						spellComp.facingDirection = lastInputDirection[entityId];
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

						// Track wall push state (no sound)
						if ((onWallLeft || onWallRight) && 
							((onWallLeft && (movingLeft || movementComp.velocity.x < -10.0f)) ||
							 (onWallRight && (movingRight || movementComp.velocity.x > 10.0f)) ||
							 (wasFalling && !onGround)))
						{
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
						if (entityManager.template HasComponent<LocationComponent>(entityId) &&
							entityManager.template HasComponent<SpellComponent>(entityId))
						{
							auto &playerLoc = entityManager.template GetComponent<LocationComponent>(entityId);
							auto &spellComp = entityManager.template GetComponent<SpellComponent>(entityId);
							
							// Use last input direction for facing (sprite flip already updated above)
							int facingDir = lastInputDirection[entityId]; // Use last input direction
								
							// Check cooldown
							if (spellComp.castCooldown <= 0.0f)
								{
									// Use the currently selected spell type
									SpellType currentSpell = spellComp.selectedSpell;
									const auto &props = spellComp.spellProperties[static_cast<size_t>(currentSpell)];
									
									// Calculate spawn position - spawn far enough to avoid colliding with player
									float spawnOffsetX = facingDir * 40.0f;
									Point2D spawnPos = {
										playerLoc.position.x + spawnOffsetX,
										playerLoc.position.y
									};
									
									// Create projectile entity
									std::string spellName = this->GetSpellEntityName(currentSpell);
									EntityID projectileId = entityManager.CreateEntity(spellName);

									entityManager.template AddComponent<LocationComponent>(projectileId, LocationComponent(spawnPos));

									MovementComponent projectileMovement;
									projectileMovement.velocity = Point2D(facingDir * props.speed, 0.0f);
									entityManager.template AddComponent<MovementComponent>(projectileId, projectileMovement);

									ProjectileComponent projectile;
									projectile.spellType = currentSpell;
									projectile.damage = props.damage;
									projectile.lifetime = props.lifetime;
									projectile.maxLifetime = props.lifetime;
									projectile.ownerEntityId = entityId;
									projectile.active = true; // active by default
									projectile.explosionFrames = props.explosionFrames;
									projectile.explosionSize = props.explosionSize;
									entityManager.template AddComponent<ProjectileComponent>(projectileId, projectile);

									// Adjust collision box position based on facing direction
									// When facing right (not flipped), offset left to align with sprite
									float collisionOffsetX = (facingDir > 0) ? -props.size : 0.0f;
									Rect collisionBounds(collisionOffsetX, 0.0f, props.size, props.size);
									CollisionComponent collision(collisionBounds, false);
									entityManager.template AddComponent<CollisionComponent>(projectileId, collision);

									// Create projectile sprite
									SpriteComponent projectileSprite;
									projectileSprite.spriteId = props.spriteId;
									projectileSprite.bounds = Rect(0.0f, 0.0f, props.size, props.size);
									projectileSprite.inWorldSpace = true;
									projectileSprite.layer = 2; // Spells should be on layer 2 (just above world)
									// Flip projectile sprite based on facing direction
									projectileSprite.flipX = (facingDir < 0);
									entityManager.template AddComponent<SpriteComponent>(projectileId, projectileSprite);

									// Add flying animation if frames exist
									if (!props.flyingFrames.empty())
									{
										AnimationComponent projectileAnim;
										projectileAnim.animations["flying"] = props.flyingFrames;
										projectileAnim.currentAnimation = "flying";
										projectileAnim.frameDuration = 0.05f;
										projectileAnim.playing = true;
										projectileAnim.looping = true;
										entityManager.template AddComponent<AnimationComponent>(projectileId, projectileAnim);
									}

									// Start cooldown
									spellComp.castCooldown = props.cooldown;

									// Play cast sound based on selected element
									PlayCastSound(currentSpell, soundManager);
								}
							}
						}

					// Handle animation transitions back to idle/run
					// Skip if we just triggered an action this frame
					// Also skip if player is dead or dying
					bool isDeadOrDying = false;
					if (entityManager.template HasComponent<HpComponent>(entityId))
					{
						auto &hpComp = entityManager.template GetComponent<HpComponent>(entityId);
						isDeadOrDying = !hpComp.isAlive || hpComp.isDying;
					}
					
					if (!justJumped && !justCast && !isDeadOrDying && entityManager.template HasComponent<AnimationComponent>(entityId))
					{
						auto &anim = entityManager.template GetComponent<AnimationComponent>(entityId);
						
						// Don't interrupt death animation
						if (anim.currentAnimation == "death")
						{
							// Skip all animation transitions during death
						}
						else
						{
							// If casting animation is done
							bool castingDone = castAnimTimer[entityId] <= 0.0f;
							bool isCasting = anim.currentAnimation == "magic" && !castingDone;
							// Consider jumping if in air OR if we recently jumped (to catch first frame)
							bool isJumping = anim.currentAnimation == "jump" && (!onGround || recentlyJumped[entityId]);
							// If hurt animation finished (not playing and not looping), return to idle/run
							bool hurtFinished = anim.currentAnimation == "hurt" && !anim.playing;
							
							// Return to idle/run after hurt animation finishes
							if (hurtFinished)
							{
								if (onGround && (movingLeft || movingRight))
								{
									// Running on ground
									if (anim.animations.count("run") > 0)
									{
										anim.Play("run", true);
									}
								}
								else if (onGround)
								{
									// Idle on ground
									if (anim.animations.count("idle") > 0)
									{
										anim.Play("idle", true);
									}
								}
							}
							
							// Don't interrupt casting, jumping, or hurt animations
							if (!isCasting && !isJumping && !hurtFinished)
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
			}

			return true;
		}

	private:
		// Get entity name for spell type
		std::string GetSpellEntityName(SpellType type)
		{
			switch (type)
			{
			case SpellType::Fire:
				return "fireball";
			case SpellType::Water:
				return "waterball";
			case SpellType::Wind:
				return "windball";
			case SpellType::Earth:
				return "rockarrow";
			default:
				return "spell";
			}
		}

		// Play cast sound based on spell type
		void PlayCastSound(SpellType type, SoundManager &soundManager)
		{
			switch (type)
			{
			case SpellType::Fire:
				{
					static std::mt19937 rng(std::random_device{}());
					std::uniform_int_distribution<int> dist(1, 2);
					std::string soundName = "fire_cast_" + std::to_string(dist(rng));
					soundManager.PlaySound(soundName);
				}
				break;
			case SpellType::Water:
				soundManager.PlaySound("water_cast");
				break;
			case SpellType::Wind:
				soundManager.PlaySound("wind_cast");
				break;
			case SpellType::Earth:
				soundManager.PlaySound("earth_cast");
				break;
			default:
				break;
			}
		}
	};

} // namespace ECSEngine
