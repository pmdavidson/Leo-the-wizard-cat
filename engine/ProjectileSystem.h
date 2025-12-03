#pragma once

#include "SystemManager.h"
#include "Scene.h"
#include "ProjectileComponent.h"
#include "SpellComponent.h"
#include "LocationComponent.h"
#include "CollisionComponent.h"
#include "SoundManager.h"
#include <vector>

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

            // Collect projectiles to remove 
            std::vector<EntityID> projectilesToRemove;

            for (auto it = entityManager.begin(); it != entityManager.end(); ++it)
            {
                if (!it->isActive())
                    continue;
                EntityID entityId = it->getID();

                if (!entityManager.template HasComponent<ProjectileComponent>(entityId))
                    continue;

                auto &projectile = entityManager.template GetComponent<ProjectileComponent>(entityId);

                // Update projectile lifetime
                if (projectile.lifetime > 0.0f)
                    projectile.lifetime -= deltaTime;

                // Check if projectile has expired
                if (projectile.lifetime <= 0.0f || !projectile.active)
                {
                    projectilesToRemove.push_back(entityId);
                    continue;
                }

                // Check collision flags set by CollisionSystem
                if (entityManager.template HasComponent<CollisionComponent>(entityId))
                {
                    auto &collision = entityManager.template GetComponent<CollisionComponent>(entityId);

                    // If projectile hit something (flags set by CollisionSystem)
                    if (collision.collidedSides.left || collision.collidedSides.right ||
                        collision.collidedSides.top || collision.collidedSides.bottom)
                    {
                        // Deactivate projectile
                        projectile.active = false;
                        projectilesToRemove.push_back(entityId);

                        // Play impact sound
                        PlayImpactSound(projectile.spellType, soundManager);
                    }
                }
            }

            // Remove expired/hit projectiles
            for (EntityID id : projectilesToRemove)
            {
                entityManager.DestroyEntity(id);
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
                soundManager.PlaySound("fire_impact");
                break;
            case SpellType::Water:
                soundManager.PlaySound("water_impact");
                break;
            case SpellType::Wind:
                soundManager.PlaySound("wind_impact");
                break;
            case SpellType::Earth:
                soundManager.PlaySound("earth_impact");
                break;
            default:
                break;
            }
        }
    };

} // namespace ECSEngine
