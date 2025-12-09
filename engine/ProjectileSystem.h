#pragma once

#include "SystemManager.h"
#include "Scene.h"
#include "ProjectileComponent.h"
#include "SpellComponent.h"
#include "LocationComponent.h"
#include "CollisionComponent.h"

#include "SpriteComponent.h"
#include "AnimationComponent.h"
#include "SoundManager.h"
#include <vector>
#include <random>
#include <unordered_map>

namespace ECSEngine
{

    template <typename... Components>
    class ProjectileSystem : public System<Components...>
    {
    public:
        bool Run(Scene<Components...> &scene) override
        {
            auto &entityManager = scene.GetEntityManager();
            auto &soundManager = scene.GetSoundManager();
            float deltaTime = 1.0f / 60.0f;

            // Track explosion lifetimes (entity ID -> remaining time)
            static std::unordered_map<EntityID, float> explosionTimers;

            // Collect entities to remove and explosions to spawn
            std::vector<EntityID> entitiesToRemove;
            struct ExplosionSpawn
            {
                Point2D position;
                std::vector<SpriteID> frames;
                float size;
                SpellType type;
            };
            std::vector<ExplosionSpawn> explosionsToSpawn;

            // Update explosion timers and remove expired ones
            for (auto it = explosionTimers.begin(); it != explosionTimers.end();)
            {
                it->second -= deltaTime;
                if (it->second <= 0.0f)
                {
                    entitiesToRemove.push_back(it->first);
                    it = explosionTimers.erase(it);
                }
                else
                {
                    ++it;
                }
            }

            for (auto it = entityManager.begin(); it != entityManager.end(); ++it)
            {
                if (!it->isActive())
                    continue;
                EntityID entityId = it->getID();

                if (!entityManager.template HasComponent<ProjectileComponent>(entityId))
                    continue;

                auto &projectile = entityManager.template GetComponent<ProjectileComponent>(entityId);

                // Update grace period (ignore collisions briefly after spawn)
                if (projectile.gracePeriod > 0.0f)
                    projectile.gracePeriod -= deltaTime;

                // Update projectile lifetime
                if (projectile.lifetime > 0.0f)
                    projectile.lifetime -= deltaTime;

                // Get projectile position for explosion spawn
                Point2D explosionPos(0, 0);
                if (entityManager.template HasComponent<LocationComponent>(entityId))
                {
                    explosionPos = entityManager.template GetComponent<LocationComponent>(entityId).position;
                }

                // Check if projectile has expired (lifetime ran out)
                if (projectile.lifetime <= 0.0f && projectile.active)
                {
                    PlayImpactSound(projectile.spellType, soundManager);
                    projectile.active = false;

                    // Queue explosion spawn
                    if (!projectile.explosionFrames.empty())
                    {
                        explosionsToSpawn.push_back({explosionPos, projectile.explosionFrames, projectile.explosionSize, projectile.spellType});
                    }

                    entitiesToRemove.push_back(entityId);
                    continue;
                }

                // Check if projectile was deactivated elsewhere (hit enemy)
                if (!projectile.active)
                {
                    // Queue explosion spawn
                    if (!projectile.explosionFrames.empty())
                    {
                        explosionsToSpawn.push_back({explosionPos, projectile.explosionFrames, projectile.explosionSize, projectile.spellType});
                    }

                    entitiesToRemove.push_back(entityId);
                    continue;
                }

                // Check collision flags set by CollisionSystem (only after grace period)
                if (projectile.gracePeriod <= 0.0f && entityManager.template HasComponent<CollisionComponent>(entityId))
                {
                    auto &collision = entityManager.template GetComponent<CollisionComponent>(entityId);

                    // If projectile hit something (flags set by CollisionSystem)
                    if (collision.collidedSides.left || collision.collidedSides.right ||
                        collision.collidedSides.top || collision.collidedSides.bottom)
                    {
                        projectile.active = false;
                        PlayImpactSound(projectile.spellType, soundManager);

                        // Queue explosion spawn
                        if (!projectile.explosionFrames.empty())
                        {
                            explosionsToSpawn.push_back({explosionPos, projectile.explosionFrames, projectile.explosionSize, projectile.spellType});
                        }

                        entitiesToRemove.push_back(entityId);
                    }
                }
            }

            // Spawn explosion entities
            for (const auto &exp : explosionsToSpawn)
            {
                EntityID explosionId = entityManager.CreateEntity("explosion");

                // Center explosion on projectile position
                Point2D centeredPos(
                    exp.position.x - exp.size / 2.0f,
                    exp.position.y - exp.size / 2.0f
                );
                entityManager.template AddComponent<LocationComponent>(explosionId, LocationComponent(centeredPos));

                // Add sprite component
                SpriteComponent explosionSprite;
                explosionSprite.spriteId = exp.frames[0];
                explosionSprite.bounds = Rect(0.0f, 0.0f, exp.size, exp.size);
                explosionSprite.inWorldSpace = true;
                entityManager.template AddComponent<SpriteComponent>(explosionId, explosionSprite);

                // Add animation component
                AnimationComponent explosionAnim;
                explosionAnim.animations["explosion"] = exp.frames;
                explosionAnim.currentAnimation = "explosion";
                explosionAnim.frameDuration = 0.05f;
                explosionAnim.playing = true;
                explosionAnim.looping = false;
                entityManager.template AddComponent<AnimationComponent>(explosionId, explosionAnim);

                // Track explosion lifetime
                float lifetime = explosionAnim.frameDuration * exp.frames.size();
                explosionTimers[explosionId] = lifetime;
            }

            // Remove expired projectiles and explosions
            for (EntityID id : entitiesToRemove)
            {
                entityManager.RemoveEntity(id);
            }

            return true;
        }

    private:
        // Play impact sound based on spell type
        void PlayImpactSound(SpellType type, SoundManager &soundManager)
        {
            switch (type)
            {
            case SpellType::Fire:
                // Randomly pick between two fire impact sounds for variety
                {
                    static std::mt19937 rng(std::random_device{}());
                    std::uniform_int_distribution<int> dist(1, 2);
                    std::string soundName = "fire_impact_" + std::to_string(dist(rng));
                    soundManager.PlaySound(soundName);
                }
                break;
            case SpellType::Water:
                soundManager.PlaySound("water_impact", 150.0f);
                break;
            case SpellType::Wind:
                soundManager.PlaySound("wind_impact");
                break;
            case SpellType::Earth:
                soundManager.PlaySound("earth_impact", 150.0f);
                break;
            default:
                break;
            }
        }
    };

} // namespace ECSEngine
