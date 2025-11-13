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

ECSEngine::Rect FromSFML(const sf::IntRect& r) {
	return {
		{ static_cast<float>(r.position.x), static_cast<float>(r.position.y) },
		r.size.x,
		r.size.y
	};
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
			sf::Vector2i(sw, sh)
		);

		if (dictionaryType == 2) {
			int bx, by, bw, bh;
			ss >> bx >> by >> bw >> bh;

			entry.boundsRect = sf::IntRect(
			sf::Vector2i(bx, by),
			sf::Vector2i(bw, bh)
			);

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

<<<<<<< HEAD
	ECSEngine::Rect FromSFML(const sf::IntRect &r)
	{
		return {
			{static_cast<float>(r.left), static_cast<float>(r.top)},
			static_cast<float>(r.width),
			static_cast<float>(r.height)};
	}

=======
>>>>>>> 41e2abee0d36944f1f0609b7eabe879a11fcc55a
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
				continue;

			const SpriteEntry &entry = dictionary[tile];

			// ECSEngine::Point2D position = { originX + x * tileW, originY + y * tileH }; //change to Point2D cause LocationComponent takes Point2D
			
			ECSEngine::Point2D position = {
				static_cast<float>(originX + x * tileW),
				static_cast<float>(originY + y * tileH)
			};



			EntityId id = engine.GetEntityManager().CreateEntity("tile_" + std::string(1, tile));

			engine.GetEntityManager().template AddComponent<ECSEngine::LocationComponent>(id, ECSEngine::LocationComponent(position));

			auto spriteId = engine.GetSpriteManager().RegisterTexture(
				resourceRoot + entry.texturePath, FromSFML(entry.sourceRect)
			);

<<<<<<< HEAD
			engine.GetEntityManager().template AddComponent<ECSEngine::SpriteComponent>(id, {spriteId, FromSFML(entry.boundsRect), true}); // cast entry.boundsRect from IntRect to Rect? causse sprite and collision component constructor takes Rect
=======
			engine.GetEntityManager().template AddComponent<ECSEngine::SpriteComponent>(id, {
				spriteId, FromSFML(entry.boundsRect), true
			}); //cast entry.boundsRect from IntRect to Rect? causse sprite and collision component constructor takes Rect
>>>>>>> 41e2abee0d36944f1f0609b7eabe879a11fcc55a

			if (entry.hasCollision) {
				engine.GetEntityManager().template AddComponent<ECSEngine::CollisionComponent>(id, ECSEngine::CollisionComponent(
					FromSFML(entry.boundsRect))
				);
			}

<<<<<<< HEAD
			// Spawner
			if (tile == 'S')
			{
=======
			//Spawner
			if (tile == 'S') {
>>>>>>> 41e2abee0d36944f1f0609b7eabe879a11fcc55a
				engine.GetEntityManager().template AddComponent<ECSEngine::SpawnComponent>(id, {id, "star", spriteId, 2.f, 2.f, 10, static_cast<float>(tileW), static_cast<float>(tileH)});
			}
		}
	}

	// Spawn position assuming (1,8) is safe
	float spawnX = tileW * 1;
	float spawnY = tileH * 8;

	// Create player entity
	EntityId player = engine.GetEntityManager().CreateEntity("player");

	// Register player sprite
	SpriteID playerSpriteId = engine.GetSpriteManager().RegisterTexture(
    	gResourcePath + "spritesheet-characters-default.png", ECSEngine::Rect(0.f, 0.f, 64.f, 64.f));


	// Add Components
<<<<<<< HEAD
	engine.GetEntityManager().AddComponent<LocationComponent>(player, {ECSEngine::Point2D(spawnX, spawnY)});
	engine.GetEntityManager().AddComponent<GravityComponent>(player, {});
	engine.GetEntityManager().AddComponent<MovementComponent>(player, {});
	engine.GetEntityManager().AddComponent<InputComponent>(player, {});
	engine.GetEntityManager().AddComponent<CameraFollower>(player, {player});
	engine.GetEntityManager().AddComponent<ScoreComponent>(player, {});
	engine.GetEntityManager().AddComponent<CollisionComponent>(player, {ECSEngine::Rect(0.f, 0.f, 64, 64), false});
	engine.GetEntityManager().AddComponent<SpriteComponent>(player, {playerSpriteId, ECSEngine::Rect(0.f, 0.f, 64, 64), true});
=======
	engine.GetEntityManager().template AddComponent<ECSEngine::LocationComponent>(player, ECSEngine::LocationComponent(ECSEngine::Point2D(spawnX, spawnY)));
	engine.GetEntityManager().template AddComponent<ECSEngine::GravityComponent>(player, {});
	engine.GetEntityManager().template AddComponent<ECSEngine::MovementComponent>(player, {});
	engine.GetEntityManager().template AddComponent<ECSEngine::InputComponent>(player, {});
	engine.GetEntityManager().template AddComponent<ECSEngine::CameraFollower>(player, {player});
	engine.GetEntityManager().template AddComponent<ECSEngine::ScoreComponent>(player, {});
	engine.GetEntityManager().template AddComponent<ECSEngine::CollisionComponent>(player, {ECSEngine::Rect(0.f, 0.f, 64, 64), false});
	engine.GetEntityManager().template AddComponent<ECSEngine::SpriteComponent>(player, {playerSpriteId, ECSEngine::Rect(0.f, 0.f, 64, 64), true });

	//register sounds 
	engine.GetSoundManager().RegisterSound(gResourcePath + "footstep_grass_003.ogg", "land");
	engine.GetSoundManager().RegisterSound(gResourcePath + "sfx_jump.ogg", "jump");
	engine.GetSoundManager().RegisterSound(gResourcePath + "footstep_snow_001.ogg", "wall_push");
	engine.GetSoundManager().RegisterSound(gResourcePath + "sfx_gem.ogg", "star_collect");

>>>>>>> 41e2abee0d36944f1f0609b7eabe879a11fcc55a
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

	// Debug entity manager test
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

	// Load maps (order matters: sky → world)
	LoadMap("sky.map", engine, gResourcePath);
	LoadMap("world.map", engine, gResourcePath);

	// Start game loop
	engine.Run();
	return 0;
}
