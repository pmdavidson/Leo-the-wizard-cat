#pragma once

#include "MathUtil.h"
#include "SpriteManager.h"
#include "SpellComponent.h"
#include <array>

namespace ECSEngine
{

    // Enum for enemy types
    enum class EnemyType
    {
        Slime = 0,
        ElementalWizard = 1,
        Boss = 2,
        Count = 3
    };

    // Component for all enemy entities
    struct EnemyComponent
    {
        // Enemy type
        EnemyType type = EnemyType::Slime; // Just for now

        // Health
        float hp = 50.0f;
        float maxHp = 50.0f;
        float previousHp = 50.0f; // Track HP changes for hurt animation

        // Damage flash effect
        float damageFlashTimer = 0.0f;
        float damageFlashDuration = 0.2f;

        // Damage dealt to player when touched
        float contactDamage = 1.0f;

        float knockbackForce = 300.0f;

        bool isAlive = true;
        bool isDying = false; // Playing death animation
        float deathAnimTimer = 0.0f; // Time until removal after death

        // For wizards - which element they use
        SpellType element = SpellType::Fire;

        // Damage multipliers per spell type (1.0 = normal, <1 = resistant)
        std::array<float, static_cast<size_t>(SpellType::Count)> resistances{};

        // Movement (for future use, enemies can move)
        bool canMove = false;
        float moveSpeed = 50.0f;
        
        // Random movement state for slimes
        float moveDirection = 1.0f; // 1.0 = right, -1.0 = left
        float directionChangeTimer = 0.0f; // Timer until next direction change
        float directionChangeInterval = 2.0f; // Change direction every 2 seconds

        // Sound names for this enemy
        std::string damageSoundName = "slime_damage";
        std::string deathSoundName = "slime_die";
    };

} // namespace ECSEngine

