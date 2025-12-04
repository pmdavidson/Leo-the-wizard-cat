#pragma once

#include <vector>
#include "MathUtil.h"
#include "SpriteManager.h"
#include "Entity.h"

namespace ECSEngine
{

    struct HpComponent
    {
        // Health values
        int currentHp = 3;
        int maxHp = 3;

        // Invincibility frames after being hit
        float invincibilityTimer = 0.0f;
        float invincibilityDuration = 1.0f;

        // Sprite IDs for hearts
        SpriteID heartFullSpriteId;  // heart_idle
        SpriteID heartEmptySpriteId; // heart_empty

        // Entity IDs for the heart display (one per max HP)
        std::vector<EntityID> heartDisplayEntityIds;

        // Flag to check if player is alive
        bool isAlive = true;

        HpComponent() = default;

        HpComponent(int hp, SpriteID fullHeart, SpriteID emptyHeart)
            : currentHp(hp), maxHp(hp), heartFullSpriteId(fullHeart), heartEmptySpriteId(emptyHeart) {}
    };

} // namespace ECSEngine

