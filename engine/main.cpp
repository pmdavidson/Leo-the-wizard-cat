#include <SFML/Graphics.hpp>
#include <SFML/Graphics/Rect.hpp>
#include "EntityManager.h"
#include "ECSEngine.h"
#include "Scene.h"
#include "MathUtil.h"
#include "SpriteManager.h"
#include "InputComponent.h"
#include "LocationComponent.h"
#include "CollisionComponent.h"
#include "SpriteComponent.h"
#include "SpawnComponent.h"
#include "ScoreComponent.h"
#include "ProcessEventsSystem.h"
#include "CollisionUpdateSystem.h"
#include "InputSystem.h"
#include "GravitySystem.h"
#include "MovementSystem.h"
#include "CollisionSystem.h"
#include "ScoreSystem.h"
#include "CameraSystem.h"
#include "SpriteSystem.h"
#include "SpawnSystem.h"
#include "SpellComponent.h"
#include "ProjectileComponent.h"
#include "SpellSystem.h"
#include "ProjectileSystem.h"
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

template <typename SceneType>
void LoadMap(const std::string &path, SceneType &scene, const std::string &resourceRoot)
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
	SpriteID starSpriteId = scene.GetSpriteManager().RegisterTexture(
		resourceRoot + "spritesheet-tiles-default.png", ECSEngine::Rect(640.f, 320.f, 32.f, 32.f));

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

			EntityId id = scene.GetEntityManager().CreateEntity("tile_" + std::string(1, tile));

			scene.GetEntityManager().template AddComponent<ECSEngine::LocationComponent>(id, ECSEngine::LocationComponent(position));

			// Drawing Sprites
			auto spriteId = scene.GetSpriteManager().RegisterTexture(
				resourceRoot + entry.texturePath, FromSFML(entry.sourceRect));

			scene.GetEntityManager().template AddComponent<ECSEngine::SpriteComponent>(id, {spriteId, FromSFML(entry.boundsRect), true});

			// Every object in the world map should be subject to collision
			if (entry.hasCollision)
			{
				scene.GetEntityManager().template AddComponent<ECSEngine::CollisionComponent>(id, ECSEngine::CollisionComponent(
																									  FromSFML(entry.boundsRect), true));
			}

			// Spawner
			if (tile == 'S')
			{
				scene.GetEntityManager().template AddComponent<ECSEngine::SpawnComponent>(id, {id, "star", starSpriteId, 10.f, 10.f, 10, static_cast<float>(tileW), static_cast<float>(tileH)});
			}
		}
	}

	// Make Player after everything else has been made
	if (dictionaryType == 2)
	{
		// Spawn position assuming (1,8) is safe
		float spawnX = tileW * 1;
		float spawnY = tileH * 1;

		// Create player entity
		EntityId player = scene.GetEntityManager().CreateEntity("player");

		// Register player sprite
		SpriteID playerSpriteId = scene.GetSpriteManager().RegisterTexture(
			gResourcePath + "sprite-sheet-character.png", ECSEngine::Rect(0.f, 32.f, 32.f, 32.f));

		// Add Components
		scene.GetEntityManager().template AddComponent<ECSEngine::LocationComponent>(player, ECSEngine::LocationComponent(ECSEngine::Point2D(spawnX, spawnY)));
		scene.GetEntityManager().template AddComponent<ECSEngine::GravityComponent>(player, ECSEngine::GravityComponent(ECSEngine::Point2D(0.0f, 600.0f)));
		scene.GetEntityManager().template AddComponent<ECSEngine::MovementComponent>(player, {});
		scene.GetEntityManager().template AddComponent<ECSEngine::InputComponent>(player, {});
		scene.GetEntityManager().template AddComponent<ECSEngine::CameraFollower>(player, {player});
		scene.GetEntityManager().template AddComponent<ECSEngine::ScoreComponent>(player, {});

		// Collision box: 32x32, full sprite size (adjust if sprite has padding/transparency)
		scene.GetEntityManager().template AddComponent<ECSEngine::CollisionComponent>(player, {ECSEngine::Rect(0.f, 0.f, 32.f, 32.f), false});

		// Sprite display bounds: show the full 32x32 sprite
		scene.GetEntityManager().template AddComponent<ECSEngine::SpriteComponent>(player, {playerSpriteId, ECSEngine::Rect(0.f, 0.f, 32.f, 32.f), true});

		// Set up spells
		// Register spell sprites (using placeholder positions - adjust based on your spritesheet)
		// You can replace these with actual spell sprite positions from your spritesheet
		SpriteID fireSpellSpriteId = scene.GetSpriteManager().RegisterTexture(
			gResourcePath + "spritesheet-tiles-default.png", ECSEngine::Rect(0.f, 0.f, 32.f, 32.f));
		SpriteID waterSpellSpriteId = scene.GetSpriteManager().RegisterTexture(
			gResourcePath + "spritesheet-tiles-default.png", ECSEngine::Rect(32.f, 0.f, 32.f, 32.f));
		SpriteID windSpellSpriteId = scene.GetSpriteManager().RegisterTexture(
			gResourcePath + "spritesheet-tiles-default.png", ECSEngine::Rect(64.f, 0.f, 32.f, 32.f));
		SpriteID earthSpellSpriteId = scene.GetSpriteManager().RegisterTexture(
			gResourcePath + "spritesheet-tiles-default.png", ECSEngine::Rect(96.f, 0.f, 32.f, 32.f));

		// Create SpellComponent for player (pure data)
		ECSEngine::SpellComponent spellComp;

		// Fire spell: high damage, fast, short cooldown
		spellComp.spellProperties[static_cast<size_t>(ECSEngine::SpellType::Fire)] = {
			15.0f,  // damage
			400.0f, // speed
			0.5f,   // cooldown
			2.0f,   // lifetime
			32.0f,  // size
			fireSpellSpriteId
		};

		// Water spell: medium damage, medium speed, medium cooldown
		spellComp.spellProperties[static_cast<size_t>(ECSEngine::SpellType::Water)] = {
			10.0f,  // damage
			300.0f, // speed
			0.8f,   // cooldown
			3.0f,   // lifetime
			32.0f,  // size
			waterSpellSpriteId
		};

		// Wind spell: low damage, very fast, very short cooldown
		spellComp.spellProperties[static_cast<size_t>(ECSEngine::SpellType::Wind)] = {
			5.0f,   // damage
			500.0f, // speed
			0.3f,   // cooldown
			1.5f,   // lifetime
			32.0f,  // size
			windSpellSpriteId
		};

		// Earth spell: very high damage, slow, long cooldown
		spellComp.spellProperties[static_cast<size_t>(ECSEngine::SpellType::Earth)] = {
			25.0f,  // damage
			200.0f, // speed
			1.5f,   // cooldown
			4.0f,   // lifetime
			32.0f,  // size
			earthSpellSpriteId
		};

		// Configure element switch cooldown (time between switching elements)
		spellComp.switchCooldownDuration = 0.5f;

		// Start with Fire element selected
		spellComp.selectedSpell = ECSEngine::SpellType::Fire;

		scene.GetEntityManager().template AddComponent<ECSEngine::SpellComponent>(player, spellComp);

		// Create camera entity that follows the player
		EntityId camera = scene.GetEntityManager().CreateEntity("camera");
		ECSEngine::CameraComponent cameraComp;
		cameraComp.position = ECSEngine::Point2D(spawnX, spawnY);
		cameraComp.scale = 1.0f;
		scene.GetEntityManager().template AddComponent<ECSEngine::CameraComponent>(camera, cameraComp);

		// Set up score display system
		// Get the ScoreComponent attached to the player entity
		auto &scoreComp = scene.GetEntityManager().template GetComponent<ECSEngine::ScoreComponent>(player);

		// Register digit sprites (0-9) from the spritesheet
		const float tileSize = 32.f;
		const float digitX = 13.f * tileSize;	  // X position of digit column in spritesheet
		const float digitStartY = 4.f * tileSize; // Y position where digit 0 starts

		for (int digit = 0; digit < 10; ++digit)
		{
			// Calculate Y position: digits are arranged top-to-bottom (0 at top, 9 at bottom)
			float digitY = digitStartY + (9 - digit) * tileSize;
			SpriteID digitSpriteId = scene.GetSpriteManager().RegisterTexture(
				gResourcePath + "spritesheet-tiles-default.png",
				ECSEngine::Rect(digitX, digitY, tileSize, tileSize));
			scoreComp.digitSpriteIds.push_back(digitSpriteId);
		}

		// Create 3 display entities for the score
		const float digitSize = 32.f;
		const float startX = 20.f;
		const float startY = 20.f;

		for (int i = 0; i < 3; ++i)
		{
			// Create an entity for each digit position
			EntityId digitEntity = scene.GetEntityManager().CreateEntity("score_digit_" + std::to_string(i));

			// Space by digitSize
			scene.GetEntityManager().template AddComponent<ECSEngine::LocationComponent>(
				digitEntity,
				ECSEngine::LocationComponent(ECSEngine::Point2D(startX + i * digitSize, startY)));

			// Initialize sprite component with digit 0 sprite
			scene.GetEntityManager().template AddComponent<ECSEngine::SpriteComponent>(
				digitEntity,
				{scoreComp.digitSpriteIds[0], ECSEngine::Rect(0.f, 0.f, digitSize, digitSize), false});

			scoreComp.displayEntityIds.push_back(digitEntity);
		}

		// Register sounds
		scene.GetSoundManager().RegisterSound(gResourcePath + "footstep_grass_003.ogg", "land");
		scene.GetSoundManager().RegisterSound(gResourcePath + "sfx_jump.ogg", "jump");
		scene.GetSoundManager().RegisterSound(gResourcePath + "footstep_snow_001.ogg", "wall_push");
		scene.GetSoundManager().RegisterSound(gResourcePath + "sfx_gem.ogg", "star_collect");

		// Register spell cast sounds (using existing sounds as placeholders - replace with actual spell sounds)
		scene.GetSoundManager().RegisterSound(gResourcePath + "sfx_jump.ogg", "fire_cast");
		scene.GetSoundManager().RegisterSound(gResourcePath + "sfx_jump.ogg", "water_cast");
		scene.GetSoundManager().RegisterSound(gResourcePath + "sfx_jump.ogg", "wind_cast");
		scene.GetSoundManager().RegisterSound(gResourcePath + "sfx_jump.ogg", "earth_cast");

		// Register spell impact sounds
		scene.GetSoundManager().RegisterSound(gResourcePath + "sfx_gem.ogg", "fire_impact");
		scene.GetSoundManager().RegisterSound(gResourcePath + "sfx_gem.ogg", "water_impact");
		scene.GetSoundManager().RegisterSound(gResourcePath + "sfx_gem.ogg", "wind_impact");
		scene.GetSoundManager().RegisterSound(gResourcePath + "sfx_gem.ogg", "earth_impact");

		// Register element selection sounds (played when switching elements)
		scene.GetSoundManager().RegisterSound(gResourcePath + "footstep_grass_003.ogg", "fire_select");
		scene.GetSoundManager().RegisterSound(gResourcePath + "footstep_grass_003.ogg", "water_select");
		scene.GetSoundManager().RegisterSound(gResourcePath + "footstep_grass_003.ogg", "wind_select");
		scene.GetSoundManager().RegisterSound(gResourcePath + "footstep_grass_003.ogg", "earth_select");
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
		ECSEngine::ScoreComponent,
		ECSEngine::SpellComponent,
		ECSEngine::ProjectileComponent>
		engine(1024, 768, "Spell Caster");

	// Create a scene
	auto scene = engine.MakeScene();

	// Define component list type alias for cleaner code
	using ComponentList = std::tuple<
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
		ECSEngine::ScoreComponent,
		ECSEngine::SpellComponent,
		ECSEngine::ProjectileComponent>;

	// Add systems to the scene in the correct order
	scene->GetSystemManager().AddSystem(std::make_unique<ECSEngine::ProcessEventsSystem<
		ECSEngine::LocationComponent, ECSEngine::MovementComponent, ECSEngine::CollisionComponent,
		ECSEngine::SpriteComponent, ECSEngine::SpawnComponent, ECSEngine::CameraComponent,
		ECSEngine::CameraFollower, ECSEngine::InputComponent, ECSEngine::GravityComponent,
		ECSEngine::CameraShake, ECSEngine::ScoreComponent, ECSEngine::SpellComponent,
		ECSEngine::ProjectileComponent>>());

	scene->GetSystemManager().AddSystem(std::make_unique<ECSEngine::CollisionUpdateSystem<
		ECSEngine::LocationComponent, ECSEngine::MovementComponent, ECSEngine::CollisionComponent,
		ECSEngine::SpriteComponent, ECSEngine::SpawnComponent, ECSEngine::CameraComponent,
		ECSEngine::CameraFollower, ECSEngine::InputComponent, ECSEngine::GravityComponent,
		ECSEngine::CameraShake, ECSEngine::ScoreComponent, ECSEngine::SpellComponent,
		ECSEngine::ProjectileComponent>>());

	scene->GetSystemManager().AddSystem(std::make_unique<ECSEngine::InputSystem<
		ECSEngine::LocationComponent, ECSEngine::MovementComponent, ECSEngine::CollisionComponent,
		ECSEngine::SpriteComponent, ECSEngine::SpawnComponent, ECSEngine::CameraComponent,
		ECSEngine::CameraFollower, ECSEngine::InputComponent, ECSEngine::GravityComponent,
		ECSEngine::CameraShake, ECSEngine::ScoreComponent, ECSEngine::SpellComponent,
		ECSEngine::ProjectileComponent>>());

	// Spell system - process spell casting input
	scene->GetSystemManager().AddSystem(std::make_unique<ECSEngine::SpellSystem<
		ECSEngine::LocationComponent, ECSEngine::MovementComponent, ECSEngine::CollisionComponent,
		ECSEngine::SpriteComponent, ECSEngine::SpawnComponent, ECSEngine::CameraComponent,
		ECSEngine::CameraFollower, ECSEngine::InputComponent, ECSEngine::GravityComponent,
		ECSEngine::CameraShake, ECSEngine::ScoreComponent, ECSEngine::SpellComponent,
		ECSEngine::ProjectileComponent>>());

	scene->GetSystemManager().AddSystem(std::make_unique<ECSEngine::GravitySystem<
		ECSEngine::LocationComponent, ECSEngine::MovementComponent, ECSEngine::CollisionComponent,
		ECSEngine::SpriteComponent, ECSEngine::SpawnComponent, ECSEngine::CameraComponent,
		ECSEngine::CameraFollower, ECSEngine::InputComponent, ECSEngine::GravityComponent,
		ECSEngine::CameraShake, ECSEngine::ScoreComponent, ECSEngine::SpellComponent,
		ECSEngine::ProjectileComponent>>());

	scene->GetSystemManager().AddSystem(std::make_unique<ECSEngine::MovementSystem<
		ECSEngine::LocationComponent, ECSEngine::MovementComponent, ECSEngine::CollisionComponent,
		ECSEngine::SpriteComponent, ECSEngine::SpawnComponent, ECSEngine::CameraComponent,
		ECSEngine::CameraFollower, ECSEngine::InputComponent, ECSEngine::GravityComponent,
		ECSEngine::CameraShake, ECSEngine::ScoreComponent, ECSEngine::SpellComponent,
		ECSEngine::ProjectileComponent>>());

	scene->GetSystemManager().AddSystem(std::make_unique<ECSEngine::CollisionSystem<
		ECSEngine::LocationComponent, ECSEngine::MovementComponent, ECSEngine::CollisionComponent,
		ECSEngine::SpriteComponent, ECSEngine::SpawnComponent, ECSEngine::CameraComponent,
		ECSEngine::CameraFollower, ECSEngine::InputComponent, ECSEngine::GravityComponent,
		ECSEngine::CameraShake, ECSEngine::ScoreComponent, ECSEngine::SpellComponent,
		ECSEngine::ProjectileComponent>>());

	// Projectile system - update projectile lifetime and handle impacts
	scene->GetSystemManager().AddSystem(std::make_unique<ECSEngine::ProjectileSystem<
		ECSEngine::LocationComponent, ECSEngine::MovementComponent, ECSEngine::CollisionComponent,
		ECSEngine::SpriteComponent, ECSEngine::SpawnComponent, ECSEngine::CameraComponent,
		ECSEngine::CameraFollower, ECSEngine::InputComponent, ECSEngine::GravityComponent,
		ECSEngine::CameraShake, ECSEngine::ScoreComponent, ECSEngine::SpellComponent,
		ECSEngine::ProjectileComponent>>());

	scene->GetSystemManager().AddSystem(std::make_unique<ECSEngine::ScoreSystem<
		ECSEngine::LocationComponent, ECSEngine::MovementComponent, ECSEngine::CollisionComponent,
		ECSEngine::SpriteComponent, ECSEngine::SpawnComponent, ECSEngine::CameraComponent,
		ECSEngine::CameraFollower, ECSEngine::InputComponent, ECSEngine::GravityComponent,
		ECSEngine::CameraShake, ECSEngine::ScoreComponent, ECSEngine::SpellComponent,
		ECSEngine::ProjectileComponent>>());

	scene->GetSystemManager().AddSystem(std::make_unique<ECSEngine::CameraSystem<
		ECSEngine::LocationComponent, ECSEngine::MovementComponent, ECSEngine::CollisionComponent,
		ECSEngine::SpriteComponent, ECSEngine::SpawnComponent, ECSEngine::CameraComponent,
		ECSEngine::CameraFollower, ECSEngine::InputComponent, ECSEngine::GravityComponent,
		ECSEngine::CameraShake, ECSEngine::ScoreComponent, ECSEngine::SpellComponent,
		ECSEngine::ProjectileComponent>>());

	scene->GetSystemManager().AddSystem(std::make_unique<ECSEngine::SpriteSystem<
		ECSEngine::LocationComponent, ECSEngine::MovementComponent, ECSEngine::CollisionComponent,
		ECSEngine::SpriteComponent, ECSEngine::SpawnComponent, ECSEngine::CameraComponent,
		ECSEngine::CameraFollower, ECSEngine::InputComponent, ECSEngine::GravityComponent,
		ECSEngine::CameraShake, ECSEngine::ScoreComponent, ECSEngine::SpellComponent,
		ECSEngine::ProjectileComponent>>());

	scene->GetSystemManager().AddSystem(std::make_unique<ECSEngine::SpawnSystem<
		ECSEngine::LocationComponent, ECSEngine::MovementComponent, ECSEngine::CollisionComponent,
		ECSEngine::SpriteComponent, ECSEngine::SpawnComponent, ECSEngine::CameraComponent,
		ECSEngine::CameraFollower, ECSEngine::InputComponent, ECSEngine::GravityComponent,
		ECSEngine::CameraShake, ECSEngine::ScoreComponent, ECSEngine::SpellComponent,
		ECSEngine::ProjectileComponent>>());

	// Load maps into the scene
	LoadMap("sky.map", *scene, gResourcePath);
	LoadMap("world.map", *scene, gResourcePath);

	// Push scene to engine and run
	engine.PushScene(scene);
	engine.Run();
	return 0;
}
