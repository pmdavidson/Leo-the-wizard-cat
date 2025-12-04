#pragma once

#include "SystemManager.h"
#include "Scene.h"
#include "CheckpointComponent.h"
#include "HpComponent.h"
#include "LocationComponent.h"

namespace ECSEngine
{

    // CheckpointSystem handles player respawn when they die
    // Campfire ignition is handled by CollisionSystem
    template <typename... Components>
    class CheckpointSystem : public System<Components...>
    {
    public:
        bool Run(Scene<Components...> &scene) override
        {
            HandlePlayerRespawn(scene);
            return true;
        }

    private:
        void HandlePlayerRespawn(Scene<Components...> &scene)
        {
            auto &entityManager = scene.GetEntityManager();

            for (auto it = entityManager.begin(); it != entityManager.end(); ++it)
            {
                if (!it->isActive())
                    continue;
                EntityID entityId = it->getID();

                // Find player entities (have both HpComponent and CheckpointComponent)
                if (!entityManager.template HasComponent<HpComponent>(entityId) ||
                    !entityManager.template HasComponent<CheckpointComponent>(entityId))
                    continue;

                auto &hpComp = entityManager.template GetComponent<HpComponent>(entityId);
                auto &checkpointComp = entityManager.template GetComponent<CheckpointComponent>(entityId);

                // Check if player died and needs respawn
                if (!hpComp.isAlive)
                {
                    // Respawn the player
                    RespawnPlayer(entityManager, entityId, hpComp, checkpointComp);
                }
            }
        }

        template <typename EntityManagerType>
        void RespawnPlayer(EntityManagerType &entityManager, EntityID playerId,
                           HpComponent &hpComp, CheckpointComponent &checkpointComp)
        {
            // Reset HP
            hpComp.currentHp = hpComp.maxHp;
            hpComp.isAlive = true;
            hpComp.invincibilityTimer = hpComp.invincibilityDuration; // Brief invincibility after respawn

            // Determine respawn position
            Point2D respawnPos = checkpointComp.lastCheckpointIndex >= 0
                                     ? checkpointComp.respawnPosition
                                     : checkpointComp.initialSpawnPosition;

            // Move player to respawn position
            if (entityManager.template HasComponent<LocationComponent>(playerId))
            {
                auto &location = entityManager.template GetComponent<LocationComponent>(playerId);
                location.position = respawnPos;
            }

            // Reset velocity
            if (entityManager.template HasComponent<MovementComponent>(playerId))
            {
                auto &movement = entityManager.template GetComponent<MovementComponent>(playerId);
                movement.velocity = Point2D(0.f, 0.f);
            }
        }
    };

} // namespace ECSEngine

