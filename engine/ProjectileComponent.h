#pragma once

#include "MathUtil.h"
#include "SpellComponent.h"
#include "Entity.h"

namespace ECSEngine
{

    // Pure data component for spell projectiles
    struct ProjectileComponent
    {
        SpellType spellType = SpellType::Fire;  // Type of spell (fire, water, wind, earth)
        float damage = 0.0f;                     // Damage dealt on hit
        float lifetime = 0.0f;                   // Time remaining before projectile expires
        float maxLifetime = 0.0f;                // Original lifetime for effects
        EntityID ownerEntityId = 0;              // Entity that cast the spell (to avoid self-damage)
        bool active = true;                      // Whether the projectile is still active
    };

}
