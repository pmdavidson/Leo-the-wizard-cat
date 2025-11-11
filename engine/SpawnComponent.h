#pragma once

#include <unordered_map>
#include <SFML/Graphics/Texture.hpp>
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
		EntityID entityId;			  // Entity id that the component is attached to
		std::string spawnDescription; // String describing what should be spawned
		sf::Texture texture;		  // Texture of the object to be spawned
		float timeToNextSpawn;		  // Time to next spawn
		float timeBetweenSpawns;	  // Time between spawns
		int totalSpawnEvents;		  // Total number of spawning events that will be performed
	};

}
