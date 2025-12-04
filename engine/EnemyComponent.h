#pragma once

#include "MathUtil.h"
#include "SpriteManager.h"
#include "SpellComponent.h"

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

        // Damage dealt to player when touched
        float contactDamage = 1.0f;

        float knockbackForce = 300.0f;

        bool isAlive = true;

        // For wizards - which element they use
        SpellType element = SpellType::Fire;

        // Movement (for future use, enemies can move)
        bool canMove = false;
        float moveSpeed = 50.0f;

        // Invincibility frames after being hit 
        float invincibilityTimer = 0.0f;
        float invincibilityDuration = 0.3f;

        // Sound names for this enemy
        std::string damageSoundName = "slime_damage";
        std::string deathSoundName = "slime_die";
    };

} // namespace ECSEngine

