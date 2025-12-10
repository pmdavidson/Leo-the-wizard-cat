#pragma once

#include "SystemManager.h"
#include "Scene.h"
#include "HpComponent.h"
#include "SpriteComponent.h"
#include "LocationComponent.h"
#include "AnimationComponent.h"
#include "CheckpointComponent.h"

namespace ECSEngine
{

    template <typename... Components>
    class HpSystem : public System<Components...>
    {
    public:
        bool Run(Scene<Components...> &scene) override
        {
            auto &entityManager = scene.GetEntityManager();
            float deltaTime = 1.0f / 60.0f;

            for (auto it = entityManager.begin(); it != entityManager.end(); ++it)
            {
                if (!it->isActive())
                    continue;
                EntityID entityId = it->getID();

                if (!entityManager.template HasComponent<HpComponent>(entityId))
                    continue;

                auto &hpComp = entityManager.template GetComponent<HpComponent>(entityId);

                // Update invincibility timer
                if (hpComp.invincibilityTimer > 0.0f)
                    hpComp.invincibilityTimer -= deltaTime;

                // Update damage flash timer
                if (hpComp.damageFlashTimer > 0.0f)
                    hpComp.damageFlashTimer -= deltaTime;

                // Check if player should die
                if (hpComp.currentHp <= 0 && hpComp.isAlive)
                {
                    hpComp.isAlive = false;
                    hpComp.isDying = true;
                    // Restart immediately
                    RestartGame(entityManager, entityId, hpComp);
                }

                // Update heart display sprites based on current HP
                for (size_t i = 0; i < hpComp.heartDisplayEntityIds.size(); ++i)
                {
                    EntityID heartEntityId = hpComp.heartDisplayEntityIds[i];

                    if (!entityManager.template HasComponent<SpriteComponent>(heartEntityId))
                        continue;

                    auto &spriteComp = entityManager.template GetComponent<SpriteComponent>(heartEntityId);

                    // If this heart index is less than current HP, show full heart
                    // Otherwise show empty heart
                    if (static_cast<int>(i) < hpComp.currentHp)
                    {
                        spriteComp.spriteId = hpComp.heartFullSpriteId;
                    }
                    else
                    {
                        spriteComp.spriteId = hpComp.heartEmptySpriteId;
                    }
                }
            }

            return true;
        }

    private:
        template <typename EntityManagerType>
        void RestartGame(EntityManagerType &entityManager, EntityID playerId, HpComponent &hpComp)
        {
            // Reset HP
            hpComp.currentHp = hpComp.maxHp;
            hpComp.previousHp = hpComp.maxHp;
            hpComp.isAlive = true;
            hpComp.isDying = false;
            hpComp.deathAnimTimer = 0.0f;
            hpComp.invincibilityTimer = hpComp.invincibilityDuration; // Brief invincibility after restart

            // Reset position to initial spawn
            if (entityManager.template HasComponent<LocationComponent>(playerId))
            {
                auto &location = entityManager.template GetComponent<LocationComponent>(playerId);
                location.position = hpComp.initialSpawnPosition;
            }

            // Reset velocity
            if (entityManager.template HasComponent<MovementComponent>(playerId))
            {
                auto &movement = entityManager.template GetComponent<MovementComponent>(playerId);
                movement.velocity = Point2D(0.f, 0.f);
            }

            // Reset checkpoints
            if (entityManager.template HasComponent<CheckpointComponent>(playerId))
            {
                auto &checkpoint = entityManager.template GetComponent<CheckpointComponent>(playerId);
                checkpoint.activeCheckpoints.reset(); // Clear all checkpoints
                checkpoint.lastCheckpointIndex = -1;
                checkpoint.respawnPosition = hpComp.initialSpawnPosition;
            }

            // Return to idle animation
            if (entityManager.template HasComponent<AnimationComponent>(playerId))
            {
                auto &anim = entityManager.template GetComponent<AnimationComponent>(playerId);
                if (anim.animations.count("idle") > 0)
                {
                    anim.Play("idle", true);
                }
            }
        }
    };

} // namespace ECSEngine

