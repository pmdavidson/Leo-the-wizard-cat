#pragma once

#include "MathUtil.h"
#include "SpriteManager.h"
#include <array>

namespace ECSEngine
{

    // Enum for the 4 spell types
    enum class SpellType
    {
        Fire = 0,
        Water = 1,
        Wind = 2,
        Earth = 3,
        Count = 4
    };

    // Struct for spell properties
    struct SpellProperties
    {
        float damage = 10.0f;
        float speed = 300.0f;
        float cooldown = 1.0f;
        float lifetime = 3.0f;
        float size = 32.0f;
        SpriteID spriteId = 0;
    };

    // Used on player to track spell state
    struct SpellComponent
    {
        // Properties for each spell type
        std::array<SpellProperties, static_cast<size_t>(SpellType::Count)> spellProperties;

        // Currently selected spell element
        SpellType selectedSpell = SpellType::Fire;

        // Cooldown for casting the selected spell (0 = ready to cast)
        float castCooldown = 0.0f;

        // Cooldown for switching elements (0 = ready to switch)
        float switchCooldown = 0.0f;

        // Time required between element switches
        float switchCooldownDuration = 1.0f;

        // Direction the player is facing (1 = right, -1 = left)
        int facingDirection = 1;
    };

}
