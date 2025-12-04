#pragma once

#include "SystemManager.h"
#include "Scene.h"
#include "HpComponent.h"
#include "SpriteComponent.h"
#include "LocationComponent.h"

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

                // Check if player should die
                if (hpComp.currentHp <= 0 && hpComp.isAlive)
                {
                    hpComp.isAlive = false;
                    // Could trigger death animation/game over here
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
    };

} // namespace ECSEngine

