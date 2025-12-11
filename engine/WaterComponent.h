#pragma once

namespace ECSEngine
{

/**
 * @brief Tag component indicating that this entity represents water.
 *
 * Entities with WaterComponent are treated specially by gameplay systems.
 * For example, the player can walk on them when in water form.
 */
struct WaterComponent
{
    WaterComponent() = default;
};

} // namespace ECSEngine
