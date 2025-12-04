#pragma once

#include "SystemManager.h"
#include "Scene.h"
#include "EnemyComponent.h"
#include "LocationComponent.h"
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

            // Track enemies to remove (dead enemies)
            std::vector<EntityID> enemiesToRemove;

            for (auto it = entityManager.begin(); it != entityManager.end(); ++it)
            {
                if (!it->isActive())
                    continue;
                EntityID entityId = it->getID();

                if (!entityManager.template HasComponent<EnemyComponent>(entityId))
                    continue;

                auto &enemy = entityManager.template GetComponent<EnemyComponent>(entityId);

                // Update invincibility timer
                if (enemy.invincibilityTimer > 0.0f)
                    enemy.invincibilityTimer -= deltaTime;

                // Check if enemy should die
                if (enemy.hp <= 0.0f && enemy.isAlive)
                {
                    enemy.isAlive = false;
                    soundManager.PlaySound(enemy.deathSoundName);
                    enemiesToRemove.push_back(entityId);
                }
            }

            // Remove dead enemies
            for (EntityID id : enemiesToRemove)
            {
                entityManager.RemoveEntity(id);
            }

            return true;
        }
    };

} // namespace ECSEngine
