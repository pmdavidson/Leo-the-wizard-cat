#pragma once

#include <unordered_map>
#include <vector>
#include "MathUtil.h"
#include "SpriteManager.h"
#include "Entity.h"
#include <string>

namespace ECSEngine
{

        // The sprite rectangle is relative to the entity location in the world
        // This is only defined if an entity also has a LocationComponent.
        struct SpawnComponent
        {
                EntityID entityId;            // Entity id that the component is attached to
                std::string spawnDescription; // String describing what should be spawned
                SpriteID spriteId;            // Sprite ID of the object to be spawned
                float timeToNextSpawn;        // Time to next spawn
                float timeBetweenSpawns;      // Time between spawns
                int totalSpawnEvents;         // Total number of spawning events that will be performed
                float tileW;                  // Tile width for AABB size
                float tileH;                  // Tile height for AABB size

                // Animation data for spawned entities
                std::unordered_map<std::string, std::vector<SpriteID>> animations;

                struct SpawnVariant
                {
                        std::string description;
                        SpriteID spriteId{};
                        std::unordered_map<std::string, std::vector<SpriteID>> animations;
                };

                std::vector<SpawnVariant> variants;

                SpawnComponent() = default;

                SpawnComponent(EntityID id, const std::string &desc, SpriteID sprite, float nextSpawn, float betweenSpawns, int totalEvents, float w, float h)
                    : entityId(id),
                      spawnDescription(desc),
                      spriteId(sprite),
                      timeToNextSpawn(nextSpawn),
                      timeBetweenSpawns(betweenSpawns),
                      totalSpawnEvents(totalEvents),
                      tileW(w),
                      tileH(h) {}
        };

}