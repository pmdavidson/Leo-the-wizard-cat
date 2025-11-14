#include <SFML/Graphics.hpp>
#include <SFML/Graphics/Rect.hpp>
#include "EntityManager.h"
#include "ECSEngine.h"
#include "MathUtil.h"
#include "SpriteManager.h"
#include "InputComponent.h"
#include "LocationComponent.h"
#include "CollisionComponent.h"
#include "SpriteComponent.h"
#include "SpawnComponent.h"
#include "ScoreComponent.h"
#include <fstream>

using EntityId = size_t;
using SpriteID = size_t;

std::string gResourcePath = "../../assets/";

struct SpriteEntry
{
	std::string texturePath;
	sf::IntRect sourceRect;
	sf::IntRect boundsRect;
	bool hasCollision = false;
};

ECSEngine::Rect FromSFML(const sf::IntRect &r)
{
	return {
		{static_cast<float>(r.position.x), static_cast<float>(r.position.y)},
		r.size.x,
		r.size.y};
}

template <typename EngineType>
void LoadMap(const std::string &path, EngineType &engine, const std::string &resourceRoot)
{
	std::ifstream file(resourceRoot + path);
	if (!file)
	{
		std::cerr << "Failed to open map: " << path << "\n";
		return;
	}

	std::unordered_map<char, SpriteEntry> dictionary;
	std::string line;
	int dictionaryType = 1;

	// Parse dictionary
	while (std::getline(file, line))
	{
		if (line.starts_with("dictionary"))
		{
			std::istringstream ss(line);
			std::string dummy;
			ss >> dummy >> dictionaryType;
			continue;
		}
		if (line.starts_with("map origin"))
			break;

		std::istringstream ss(line);
		char symbol;
		SpriteEntry entry;
		int sx, sy, sw, sh;
		ss >> symbol >> entry.texturePath >> sx >> sy >> sw >> sh;
		entry.sourceRect = sf::IntRect(
			sf::Vector2i(sx, sy),
			sf::Vector2i(sw, sh));

		if (dictionaryType == 2)
		{
			int bx, by, bw, bh;
			ss >> bx >> by >> bw >> bh;

			entry.boundsRect = sf::IntRect(
				sf::Vector2i(bx, by),
				sf::Vector2i(bw, bh));

			entry.hasCollision = true;
		}

		dictionary[symbol] = entry;
	}

	// Parse map origin and size
	int originX, originY, tileW, tileH, mapW, mapH;
	{
		std::istringstream ss(line); // Last line read was "map origin ..."
		std::string dummy;
		ss >> dummy >> dummy >> originX >> originY;
		ss >> dummy >> tileW >> tileH;
		ss >> dummy >> mapW >> mapH;
	}

	// Register star sprite
	SpriteID starSpriteId = engine.GetSpriteManager().RegisterTexture(
		resourceRoot + "spritesheet-tiles-default.png", ECSEngine::Rect(640.f, 320.f, 64.f, 64.f));

	// Parse map rows
	for (int y = 0; y < mapH; ++y)
	{
		std::getline(file, line);
		for (int x = 0; x < mapW; ++x)
		{
			char tile = line[x];
			if (tile == '.')
				continue;
			if (!dictionary.contains(tile))
			{
				continue;
			}

			const SpriteEntry &entry = dictionary[tile];

			ECSEngine::Point2D position = {
				static_cast<float>(originX + x * tileW),
				static_cast<float>(originY + y * tileH)};

			EntityId id = engine.GetEntityManager().CreateEntity("tile_" + std::string(1, tile));

			engine.GetEntityManager().template AddComponent<ECSEngine::LocationComponent>(id, ECSEngine::LocationComponent(position));

			// Drawing Sprites
			auto spriteId = engine.GetSpriteManager().RegisterTexture(
				resourceRoot + entry.texturePath, FromSFML(entry.sourceRect));

			engine.GetEntityManager().template AddComponent<ECSEngine::SpriteComponent>(id, {spriteId, FromSFML(entry.boundsRect), true});

			// Every object in the world map should be subject to collision
			if (entry.hasCollision)
			{
				engine.GetEntityManager().template AddComponent<ECSEngine::CollisionComponent>(id, ECSEngine::CollisionComponent(
																									   FromSFML(entry.boundsRect), true));
			}

			// Spawner
			if (tile == 'S')
			{
				engine.GetEntityManager().template AddComponent<ECSEngine::SpawnComponent>(id, {id, "star", starSpriteId, 10.f, 10.f, 10, static_cast<float>(tileW), static_cast<float>(tileH)});
			}

		}
	}

	// Make Player after everything else has been made
	if (dictionaryType == 2){
		// Spawn position assuming (1,8) is safe
		float spawnX = tileW * 1;
		float spawnY = tileH * 1;

		// Create player entity
		EntityId player = engine.GetEntityManager().CreateEntity("player");

		// Register player sprite
		SpriteID playerSpriteId = engine.GetSpriteManager().RegisterTexture(
			gResourcePath + "spritesheet-characters-default.png", ECSEngine::Rect(0.f, 0.f, 128.f, 128.f));

		// Add Components
		engine.GetEntityManager().template AddComponent<ECSEngine::LocationComponent>(player, ECSEngine::LocationComponent(ECSEngine::Point2D(spawnX, spawnY)));
		engine.GetEntityManager().template AddComponent<ECSEngine::GravityComponent>(player, ECSEngine::GravityComponent(ECSEngine::Point2D(0.0f, 600.0f)));
		engine.GetEntityManager().template AddComponent<ECSEngine::MovementComponent>(player, {});
		engine.GetEntityManager().template AddComponent<ECSEngine::InputComponent>(player, {});
		engine.GetEntityManager().template AddComponent<ECSEngine::CameraFollower>(player, {player});
		engine.GetEntityManager().template AddComponent<ECSEngine::ScoreComponent>(player, {});
		
		// Collision box: 64x64, centered horizontally (offset by 32 from left) and positioned at bottom (offset by 64 from top)
		// This aligns with where the character's body is in the 128x128 sprite
		engine.GetEntityManager().template AddComponent<ECSEngine::CollisionComponent>(player, {ECSEngine::Rect(32.f, 64.f, 64.f, 64.f), false});
		
		// Sprite display bounds: show the full 128x128 sprite
		engine.GetEntityManager().template AddComponent<ECSEngine::SpriteComponent>(player, {playerSpriteId, ECSEngine::Rect(0.f, 0.f, 128.f, 128.f), true});

		// Create camera entity that follows the player
		EntityId camera = engine.GetEntityManager().CreateEntity("camera");
		ECSEngine::CameraComponent cameraComp;
		cameraComp.position = ECSEngine::Point2D(spawnX, spawnY);
		cameraComp.scale = 1.0f;
		engine.GetEntityManager().template AddComponent<ECSEngine::CameraComponent>(camera, cameraComp);

		// Set up score display system
		// Get the ScoreComponent attached to the player entity
		auto &scoreComp = engine.GetEntityManager().template GetComponent<ECSEngine::ScoreComponent>(player);
		
		// Register digit sprites (0-9) from the spritesheet
		const float tileSize = 64.f;
		const float digitX = 13.f * tileSize;        // X position of digit column in spritesheet
		const float digitStartY = 4.f * tileSize;    // Y position where digit 0 starts
		
		for (int digit = 0; digit < 10; ++digit)
		{
			// Calculate Y position: digits are arranged top-to-bottom (0 at top, 9 at bottom)
			float digitY = digitStartY + (9 - digit) * tileSize;
			SpriteID digitSpriteId = engine.GetSpriteManager().RegisterTexture(
				gResourcePath + "spritesheet-tiles-default.png",
				ECSEngine::Rect(digitX, digitY, tileSize, tileSize));
			scoreComp.digitSpriteIds.push_back(digitSpriteId);
		}

		// Create 3 display entities for the score
		const float digitSize = 64.f;
		const float startX = 20.f; 
		const float startY = 20.f;  

		for (int i = 0; i < 3; ++i)
		{
			// Create an entity for each digit position
			EntityId digitEntity = engine.GetEntityManager().CreateEntity("score_digit_" + std::to_string(i));
			
			// Space by digitSize
			engine.GetEntityManager().template AddComponent<ECSEngine::LocationComponent>(
				digitEntity,
				ECSEngine::LocationComponent(ECSEngine::Point2D(startX + i * digitSize, startY)));
			
			// Initialize sprite component with digit 0 sprite
			engine.GetEntityManager().template AddComponent<ECSEngine::SpriteComponent>(
				digitEntity,
				{scoreComp.digitSpriteIds[0], ECSEngine::Rect(0.f, 0.f, digitSize, digitSize), false});
			
			scoreComp.displayEntityIds.push_back(digitEntity);
		}

		// Register sounds
		engine.GetSoundManager().RegisterSound(gResourcePath + "footstep_grass_003.ogg", "land");
		engine.GetSoundManager().RegisterSound(gResourcePath + "sfx_jump.ogg", "jump");
		engine.GetSoundManager().RegisterSound(gResourcePath + "footstep_snow_001.ogg", "wall_push");
		engine.GetSoundManager().RegisterSound(gResourcePath + "sfx_gem.ogg", "star_collect");
	}
}

int main(int argc, char *argv[])
{
	bool debugMode = false;

	if (argc >= 3 && argv[1] == std::string("-path"))
	{
		gResourcePath = argv[2];
	}
	std::cout << "Usage: " << argv[0] << " [-path resource_path]\n";
	std::cout << "Using resource path: " << gResourcePath << "\n";

	if (debugMode)
	{
		ECSEngine::EntityManager<int, float, bool> e;
		ECSEngine::EntityID e1 = e.CreateEntity("test");
		e.AddComponent(e1, 42);
		e.AddComponent(e1, 3.14f);
		std::cout << e.HasComponent<bool>(e1) << "\n";
		std::cout << e.HasComponent<float>(e1) << "\n";
		std::cout << e.GetComponent<float>(e1) << "\n";
		std::cout << e.HasComponent<int>(e1) << "\n";
		e.GetComponent<int>(e1) = 31415;
		std::cout << e.GetComponent<int>(e1) << "\n";
		return 0;
	}

	// Initialize engine
	ECSEngine::ECSEngine<
		ECSEngine::LocationComponent,
		ECSEngine::MovementComponent,
		ECSEngine::CollisionComponent,
		ECSEngine::SpriteComponent,
		ECSEngine::SpawnComponent,
		ECSEngine::CameraComponent,
		ECSEngine::CameraFollower,
		ECSEngine::InputComponent,
		ECSEngine::GravityComponent,
		ECSEngine::CameraShake,
		ECSEngine::ScoreComponent>
		engine(1024, 768, "Test Engine");

	LoadMap("sky.map", engine, gResourcePath);
	LoadMap("world.map", engine, gResourcePath);

	engine.Run();
	return 0;
}
