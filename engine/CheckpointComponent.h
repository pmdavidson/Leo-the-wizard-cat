#pragma once

#include <bitset>
#include "MathUtil.h"
#include "SpriteManager.h"

namespace ECSEngine
{

    // Component attached to the player to track checkpoint state
    struct CheckpointComponent
    {
        // Bitset for checkpoint activation (4 checkpoints max)
        std::bitset<4> activeCheckpoints;

        // Last activated checkpoint position (respawn point)
        Point2D respawnPosition;

        // Index of the last activated checkpoint (-1 if none)
        int lastCheckpointIndex = -1;

        // Initial spawn position (fallback if no checkpoint active)
        Point2D initialSpawnPosition;

        CheckpointComponent() = default;

        CheckpointComponent(Point2D initialSpawn)
            : respawnPosition(initialSpawn), initialSpawnPosition(initialSpawn) {}

        // Check if a specific checkpoint is active
        bool IsCheckpointActive(size_t index) const
        {
            if (index >= 4)
                return false;
            return activeCheckpoints.test(index);
        }

        // Activate a checkpoint and update respawn position
        void ActivateCheckpoint(size_t index, Point2D position)
        {
            if (index >= 4)
                return;
            activeCheckpoints.set(index);
            respawnPosition = position;
            lastCheckpointIndex = static_cast<int>(index);
        }
    };

    // Component attached to campfire entities
    struct CampfireComponent
    {
        // Which checkpoint index this campfire represents (0-3)
        size_t checkpointIndex = 0;

        // Whether this campfire is lit
        bool isLit = false;

        // Sprite IDs for lit and unlit states
        SpriteID unlitSpriteId;
        SpriteID litSpriteId;

        CampfireComponent() = default;

        CampfireComponent(size_t index, SpriteID unlit, SpriteID lit)
            : checkpointIndex(index), isLit(false), unlitSpriteId(unlit), litSpriteId(lit) {}
    };

} // namespace ECSEngine

