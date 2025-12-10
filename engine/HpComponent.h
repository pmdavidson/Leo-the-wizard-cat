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
        int previousHp = 3; // Track HP changes for hurt animation

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

        // Death animation state
        bool isDying = false; // Playing death animation
        float deathAnimTimer = 0.0f; // Time until restart after death animation

        // Initial spawn position for game restart
        Point2D initialSpawnPosition = Point2D(0.0f, 0.0f);

        // Damage flash effect
        float damageFlashTimer = 0.0f; // Timer for red screen flash when taking damage
        float damageFlashDuration = 0.2f; // Duration of the flash effect

        // Rock shield (brown shader that absorbs 1 hit)
        bool hasRockShield = false; // True when rock element is selected and shield is active

        HpComponent() = default;

        HpComponent(int hp, SpriteID fullHeart, SpriteID emptyHeart)
            : currentHp(hp), maxHp(hp), previousHp(hp), heartFullSpriteId(fullHeart), heartEmptySpriteId(emptyHeart) {}
    };

} // namespace ECSEngine

