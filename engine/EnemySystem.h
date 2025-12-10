#pragma once

#include "SystemManager.h"
#include "Scene.h"
#include "EnemyComponent.h"
#include "LocationComponent.h"
#include "AnimationComponent.h"
#include "SpriteComponent.h"
#include "CollisionComponent.h"
#include "SoundManager.h"
#include <vector>
#include <random>
#include <unordered_map>
#include <cmath>

namespace ECSEngine
{

    template <typename... Components>
    class EnemySystem : public System<Components...>
    {
    public:
        bool Run(Scene<Components...> &scene) override
        {
            auto &entityManager = scene.GetEntityManager();
            auto &soundManager = scene.GetSoundManager();
            float deltaTime = 1.0f / 60.0f;

            // Track enemies to remove (dead enemies after death animation)
            std::vector<EntityID> enemiesToRemove;

            for (auto it = entityManager.begin(); it != entityManager.end(); ++it)
            {
                if (!it->isActive())
                    continue;
                EntityID entityId = it->getID();

                if (!entityManager.template HasComponent<EnemyComponent>(entityId))
                    continue;

                auto &enemy = entityManager.template GetComponent<EnemyComponent>(entityId);

                // Tick damage flash timer
                if (enemy.damageFlashTimer > 0.0f)
                {
                    enemy.damageFlashTimer -= deltaTime;
                    if (enemy.damageFlashTimer < 0.0f)
                        enemy.damageFlashTimer = 0.0f;
                }

                // Check if enemy took damage (for hurt animation)
                if (enemy.hp < enemy.previousHp && enemy.isAlive && !enemy.isDying)
                {
                    // Play hurt animation
                    if (entityManager.template HasComponent<AnimationComponent>(entityId))
                    {
                        auto &anim = entityManager.template GetComponent<AnimationComponent>(entityId);
                        if (anim.animations.count("hurt") > 0)
                        {
                            anim.Play("hurt", false);
                            // Set first frame immediately
                            if (entityManager.template HasComponent<SpriteComponent>(entityId))
                            {
                                auto &sprite = entityManager.template GetComponent<SpriteComponent>(entityId);
                                sprite.spriteId = anim.animations["hurt"][0];
                            }
                        }
                    }
                }
                enemy.previousHp = enemy.hp;

                // Check if enemy should start dying
                if (enemy.hp <= 0.0f && enemy.isAlive && !enemy.isDying)
                {
                    enemy.isAlive = false;
                    enemy.isDying = true;
                    soundManager.PlaySound(enemy.deathSoundName);

                    // Play death animation
                    if (entityManager.template HasComponent<AnimationComponent>(entityId))
                    {
                        auto &anim = entityManager.template GetComponent<AnimationComponent>(entityId);
                        if (anim.animations.count("death") > 0)
                        {
                            anim.Play("death", false);
                            // Set death animation timer based on animation length
                            enemy.deathAnimTimer = anim.frameDuration * anim.animations["death"].size();
                            // Set first frame immediately
                            if (entityManager.template HasComponent<SpriteComponent>(entityId))
                            {
                                auto &sprite = entityManager.template GetComponent<SpriteComponent>(entityId);
                                sprite.spriteId = anim.animations["death"][0];
                            }
                        }
                        else
                        {
                            // No death animation, remove immediately
                            enemy.deathAnimTimer = 0.0f;
                        }
                    }
                    else
                    {
                        // No animation component, remove immediately
                        enemy.deathAnimTimer = 0.0f;
                    }
                }

                // Update death animation timer
                if (enemy.isDying)
                {
                    enemy.deathAnimTimer -= deltaTime;
                    if (enemy.deathAnimTimer <= 0.0f)
                    {
                        enemiesToRemove.push_back(entityId);
                    }
                }

                // Return to idle after hurt animation finishes (if still alive)
                if (enemy.isAlive && !enemy.isDying && entityManager.template HasComponent<AnimationComponent>(entityId))
                {
                    auto &anim = entityManager.template GetComponent<AnimationComponent>(entityId);
                    // If hurt animation finished (not playing and not looping), return to idle
                    if (anim.currentAnimation == "hurt" && !anim.playing)
                    {
                        if (anim.animations.count("idle") > 0)
                        {
                            anim.Play("idle", true);
                        }
                    }
                }

                // Handle slime random movement
                if (enemy.isAlive && !enemy.isDying && enemy.type == EnemyType::Slime && enemy.canMove)
                {
                    // Check for wall collisions and reverse direction if needed
                    // Only reverse if colliding with static objects (walls)
                    static std::unordered_map<EntityID, Point2D> lastPositions;
                    if (entityManager.template HasComponent<LocationComponent>(entityId) &&
                        entityManager.template HasComponent<CollisionComponent>(entityId))
                    {
                        auto &location = entityManager.template GetComponent<LocationComponent>(entityId);
                        auto &collision = entityManager.template GetComponent<CollisionComponent>(entityId);
                        Point2D currentPos = location.position;
                        
                        if (lastPositions.find(entityId) != lastPositions.end())
                        {
                            Point2D lastPos = lastPositions[entityId];
                            float expectedMovement = enemy.moveDirection * enemy.moveSpeed * deltaTime;
                            float actualMovement = currentPos.x - lastPos.x;
                            
                            // If we're trying to move but haven't moved, we hit a wall
                            if (std::abs(actualMovement) < std::abs(expectedMovement) * 0.5f && std::abs(expectedMovement) > 0.1f)
                            {
                                // Check collision sides to determine which wall we hit
                                if (collision.collidedSides.left && enemy.moveDirection < 0.0f)
                                {
                                    enemy.moveDirection = 1.0f; // Move right
                                    enemy.directionChangeTimer = enemy.directionChangeInterval; // Reset timer
                                }
                                else if (collision.collidedSides.right && enemy.moveDirection > 0.0f)
                                {
                                    enemy.moveDirection = -1.0f; // Move left
                                    enemy.directionChangeTimer = enemy.directionChangeInterval; // Reset timer
                                }
                            }
                        }
                        lastPositions[entityId] = currentPos;
                    }
                    
                    // Update direction change timer
                    enemy.directionChangeTimer -= deltaTime;
                    
                    // Change direction randomly when timer expires
                    if (enemy.directionChangeTimer <= 0.0f)
                    {
                        static std::mt19937 rng(std::random_device{}());
                        std::uniform_real_distribution<float> dist(0.0f, 1.0f);
                        
                        // Randomly choose left or right
                        enemy.moveDirection = (dist(rng) < 0.5f) ? -1.0f : 1.0f;
                        
                        // Set next direction change time (1-3 seconds)
                        std::uniform_real_distribution<float> timeDist(1.0f, 3.0f);
                        enemy.directionChangeTimer = timeDist(rng);
                    }
                    
                    // Apply movement directly to position
                    if (entityManager.template HasComponent<LocationComponent>(entityId))
                    {
                        auto &location = entityManager.template GetComponent<LocationComponent>(entityId);
                        location.position.x += enemy.moveDirection * enemy.moveSpeed * deltaTime;
                    }
                    
                    // Flip sprite based on movement direction
                    if (entityManager.template HasComponent<SpriteComponent>(entityId))
                    {
                        auto &sprite = entityManager.template GetComponent<SpriteComponent>(entityId);
                        sprite.flipX = (enemy.moveDirection < 0.0f);
                    }
                    
                    // Play idle animation while moving
                    if (entityManager.template HasComponent<AnimationComponent>(entityId))
                    {
                        auto &anim = entityManager.template GetComponent<AnimationComponent>(entityId);
                        // Only play idle if not playing hurt or death animation
                        if (anim.currentAnimation != "hurt" && anim.currentAnimation != "death" && anim.animations.count("idle") > 0)
                        {
                            if (anim.currentAnimation != "idle")
                            {
                                anim.Play("idle", true);
                            }
                        }
                    }
                }
            }

            // Remove dead enemies after death animation
            for (EntityID id : enemiesToRemove)
            {
                entityManager.RemoveEntity(id);
            }

            return true;
        }
    };

} // namespace ECSEngine
