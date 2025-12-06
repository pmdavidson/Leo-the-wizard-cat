#pragma once

#include "SystemManager.h"
#include "Scene.h"
#include "EnemyComponent.h"
#include "LocationComponent.h"
#include "AnimationComponent.h"
#include "SpriteComponent.h"
#include "SoundManager.h"
#include <vector>

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
