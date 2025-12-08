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
#include "EnemyComponent.h"
#include "EnemySystem.h"
#include "HpComponent.h"
#include "HpSystem.h"
#include "CheckpointComponent.h"
#include "CheckpointSystem.h"
#include "AnimationComponent.h"
#include "AnimationSystem.h"
#include "SpellSystem.h"
#include "ProjectileSystem.h"

#include <fstream>

using EntityId = size_t;
using SpriteID = size_t;

std::string gResourcePath = "../../assets/";

// Game Component Type List
using GameComponents = std::tuple<
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
	ECSEngine::ProjectileComponent,
	ECSEngine::EnemyComponent,
	ECSEngine::HpComponent,
	ECSEngine::CheckpointComponent,
	ECSEngine::CampfireComponent,
	ECSEngine::AnimationComponent>;

// Helper to create ECSEngine from a tuple of components
template <typename Tuple>
struct EngineFromTuple;

template <typename... Components>
struct EngineFromTuple<std::tuple<Components...>>
{
	using type = ECSEngine::ECSEngine<Components...>;
};

using GameEngine = EngineFromTuple<GameComponents>::type;

// Helper to add systems using the GameComponents tuple
template <template <typename...> class System, typename Tuple>
struct AddSystemFromTuple;

template <template <typename...> class System, typename... Components>
struct AddSystemFromTuple<System, std::tuple<Components...>>
{
	template <typename SystemManager>
	static void Add(SystemManager &manager)
	{
		manager.AddSystem(std::make_unique<System<Components...>>());
	}
};

template <template <typename...> class System, typename SystemManager>
void AddSystem(SystemManager &manager)
{
	AddSystemFromTuple<System, GameComponents>::Add(manager);
}

// Helper to get system type from a tuple of components
template <template <typename...> class System, typename Tuple>
struct SystemTypeFromTuple;

template <template <typename...> class System, typename... Components>
struct SystemTypeFromTuple<System, std::tuple<Components...>>
{
	using type = System<Components...>;
};

// Type alias for SpriteSystem with GameComponents
using GameSpriteSystem = SystemTypeFromTuple<ECSEngine::SpriteSystem, GameComponents>::type;

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

// set layer = 2 with parallax factor
template <typename SceneType>
void LoadMap(const std::string &path, SceneType &scene)
{

	//REGISTER NON-MAP THINGS
	std::vector<std::filesystem::path> paths;
	for (const auto &f : std::filesystem::directory_iterator(std::filesystem::path{gResourcePath + "sprites/"}))
	{
		if (std::filesystem::is_regular_file(f) && f.path().extension().string() == ".png")
		{
			paths.push_back(f.path());
		}
	}

	// campfire is in map too so no need to handle here, projectiles are made dynamically not pre-made
	EntityId player = scene.GetEntityManager().CreateEntity("player");
	EntityId blueSlime = scene.GetEntityManager().CreateEntity("blueSlime");
	EntityId redSlime = scene.GetEntityManager().CreateEntity("redSlime");
	EntityId greenSlime = scene.GetEntityManager().CreateEntity("greenSlime");
	EntityId brownSlime = scene.GetEntityManager().CreateEntity("brownSlime");
	// EntityId campfire = scene.GetEntityManager().CreateEntity("campfire");

	SpriteID blueSlimeSpriteId = 0;
	bool first = true;

	for (const auto &path : paths)
		{
			std::string filename = path.stem().string(); // no .png extension
			std::vector<std::string> parts;
			std::stringstream ss(filename);
			std::string part;

			std::filesystem::path spritesDir = gResourcePath + "sprites";
			std::filesystem::path fullpath = spritesDir / path.filename();

			sf::Image original;
			if (!original.loadFromFile(fullpath))
			{
				std::cerr << "Failed to load image: " << fullpath << "\n";
				continue;
			}

			while (std::getline(ss, part, '_'))
			{
				parts.push_back(part);
			}

			// blueSlime_idle_0
			// background things have _ at the front _swamp_bg_1.png

			if (parts.size() >= 3)
			{
				std::string name = parts[0];	  // "cat" or "blueSlime"
				std::string animation = parts[1]; // "idleA", "run", "idle", "hurt", "death", etc.

				if (name == "cat")
				{
					SpriteID spriteId = scene.GetSpriteManager().RegisterTexture(
					fullpath, ECSEngine::Rect(ECSEngine::Point2D(0, 0), 32.f, 32.f));
					//is this always 32x32 or should i use original.getSize()?

					scene.GetEntityManager().template AddComponent<ECSEngine::SpriteComponent>(player, ECSEngine::SpriteComponent(spriteId, ECSEngine::Rect(ECSEngine::Point2D(0, 0), 32.f, 32.f), true, 1));

					// Add this frame to the animation
					scene.GetEntityManager().template AddComponent<ECSEngine::AnimationComponent>(player, ECSEngine::AnimationComponent(animation, SpriteID));

				}
				else if (name == "blueSlime")
				{
					SpriteID spriteId = scene.GetSpriteManager().RegisterTexture(
					fullpath, ECSEngine::Rect(ECSEngine::Point2D(0, 0), 32.f, 32.f));
					//is this always 32x32 or should i use original.getSize()?

					scene.GetEntityManager().template AddComponent<ECSEngine::SpriteComponent>(blueSlime, ECSEngine::SpriteComponent(spriteId, ECSEngine::Rect(ECSEngine::Point2D(0, 0), 32.f, 32.f), true, 1));

					scene.GetEntityManager().template AddComponent<ECSEngine::AnimationComponent>(blueSlime, ECSEngine::AnimationComponent(animation, SpriteID));

					if (first){
						blueSlimeSpriteId = spriteId;
						first = false;
					}

					//refresh in every else if since redSlime is alphabetically after blueSlime
				}
				else if (name == "redSlime")
				{
					SpriteID spriteId = scene.GetSpriteManager().RegisterTexture(
					fullpath, ECSEngine::Rect(ECSEngine::Point2D(0, 0), 32.f, 32.f));
					//is this always 32x32 or should i use original.getSize()?

					scene.GetEntityManager().template AddComponent<ECSEngine::SpriteComponent>(redSlime, ECSEngine::SpriteComponent(spriteId, ECSEngine::Rect(ECSEngine::Point2D(0, 0), 32.f, 32.f), true, 1));

					scene.GetEntityManager().template AddComponent<ECSEngine::AnimationComponent>(redSlime, ECSEngine::AnimationComponent(animation, SpriteID));
				}
				else if (name == "greenSlime")
				{
					SpriteID spriteId = scene.GetSpriteManager().RegisterTexture(
					fullpath, ECSEngine::Rect(ECSEngine::Point2D(0, 0), 32.f, 32.f));
					//is this always 32x32 or should i use original.getSize()?

					scene.GetEntityManager().template AddComponent<ECSEngine::SpriteComponent>(greenSlime, ECSEngine::SpriteComponent(spriteId, ECSEngine::Rect(ECSEngine::Point2D(0, 0), 32.f, 32.f), true, 1));

					scene.GetEntityManager().template AddComponent<ECSEngine::AnimationComponent>(greenSlime, ECSEngine::AnimationComponent(animation, SpriteID));
				}
				else if (name == "brownSlime")
				{
					SpriteID spriteId = scene.GetSpriteManager().RegisterTexture(
					fullpath, ECSEngine::Rect(ECSEngine::Point2D(0, 0), 32.f, 32.f));
					//is this always 32x32 or should i use original.getSize()?

					scene.GetEntityManager().template AddComponent<ECSEngine::SpriteComponent>(brownSlime, ECSEngine::SpriteComponent(spriteId, ECSEngine::Rect(ECSEngine::Point2D(0, 0), 32.f, 32.f), true, 1));

					scene.GetEntityManager().template AddComponent<ECSEngine::AnimationComponent>(brownSlime, ECSEngine::AnimationComponent(animation, SpriteID));
				}
				//will this be in map? if so then it shouldnt be here
				// else if (name == "campfire")
				// {
				// 	scene.GetEntityManager().template AddComponent<ECSEngine::AnimationComponent>(campfire, ECSEngine::AnimationComponent(animation, SpriteID));
				// }

				//this is dynamic not static
				// else if (name == "fireball")
				// {
				// 	// Add fireball animation frames (explosion, flying)
				// 	fireballExplosionAnim.animations[animation].push_back(spriteId);
				// }
				else {
					continue;
				}
			}
		}
	
	//REGISTER MAP THINGS HERE
	std::ifstream file(gResourcePath + path);
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

		// Skip empty lines
		if (line.empty() || line.find_first_not_of(" \t") == std::string::npos)
			continue;

		std::istringstream ss(line);
		char symbol;
		SpriteEntry entry;
		int sx, sy, sw, sh;

		// remove texture path
		if (!(ss >> symbol >> entry.texturePath >> sx >> sy >> sw >> sh))
			continue; // Skip malformed lines

		entry.texturePath = gResourcePath + "sprites/" + entry.texturePath;
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

			if (bx == 0 && by == 0 && bw == 0 && bh == 0)
			{
				entry.hasCollision = false;
			}
			else
			{
				entry.hasCollision = true;
			}
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

	// Register blueSlime sprite for spawning
	// SpriteID blueSlimeSpriteId = scene.GetSpriteManager().RegisterTexture(
	// 	resourceRoot + "sprites/blueSlime_idle_0.png", ECSEngine::Rect(0, 0, 96, 32));

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

			EntityId id = scene.GetEntityManager().CreateEntity("tile_" + std::string(1, tile)); //what is this

			scene.GetEntityManager().template AddComponent<ECSEngine::LocationComponent>(id, ECSEngine::LocationComponent(position));

			// Drawing Sprites
			auto spriteId = scene.GetSpriteManager().RegisterTexture(
				entry.texturePath, FromSFML(entry.sourceRect));

			ECSEngine::Rect drawBounds(0.f, 0.f,
									   static_cast<float>(entry.sourceRect.size.x),
									   static_cast<float>(entry.sourceRect.size.y));
			scene.GetEntityManager().template AddComponent<ECSEngine::SpriteComponent>(id, {spriteId, drawBounds, true, -2});

			// Every object in the world map should be subject to collision
			if (entry.hasCollision)
			{
				scene.GetEntityManager().template AddComponent<ECSEngine::CollisionComponent>(id, ECSEngine::CollisionComponent(
																									  FromSFML(entry.boundsRect), true));
			}

			// Spawner - spawns slimes (96x32) default is blue for now, add more for red
			if (tile == 'S')
			{
				scene.GetEntityManager().template AddComponent<ECSEngine::SpawnComponent>(id, {id, "blueSlime", blueSlimeSpriteId, 10.f, 10.f, 10, 96.f, 32.f});
			}
		}
	}

	// Make Player after everything else has been made
	if (dictionaryType == 2)
	{
		// Spawn position
		float spawnX = tileW * 8;
		float spawnY = tileH * 3;

		// Update all SpawnComponents with blueSlime animations
		for (auto it = scene.GetEntityManager().begin(); it != scene.GetEntityManager().end(); ++it)
		{
			if (!it->isActive())
				continue;
			EntityId spawnerId = it->getID();
			if (scene.GetEntityManager().template HasComponent<ECSEngine::SpawnComponent>(spawnerId))
			{
				auto &spawnComp = scene.GetEntityManager().template GetComponent<ECSEngine::SpawnComponent>(spawnerId);
				if (spawnComp.spawnDescription == "blueSlime")
				{
					spawnComp.animations = blueSlimeAnim.animations; //you need a reference to the animation component, there is no animations variable in spawn component
				}
			}
		}

		// Add Player Components
		scene.GetEntityManager().template AddComponent<ECSEngine::LocationComponent>(player, ECSEngine::LocationComponent(ECSEngine::Point2D(spawnX, spawnY)));
		scene.GetEntityManager().template AddComponent<ECSEngine::GravityComponent>(player, ECSEngine::GravityComponent(ECSEngine::Point2D(0.0f, 600.0f)));
		scene.GetEntityManager().template AddComponent<ECSEngine::MovementComponent>(player, {});
		scene.GetEntityManager().template AddComponent<ECSEngine::InputComponent>(player, {});
		scene.GetEntityManager().template AddComponent<ECSEngine::CameraFollower>(player, {player});
		scene.GetEntityManager().template AddComponent<ECSEngine::ScoreComponent>(player, {});
		scene.GetEntityManager().template AddComponent<ECSEngine::CollisionComponent>(player, {ECSEngine::Rect(0.f, 0.f, 32.f, 32.f), false}); // Collision box: 32x32, full sprite size (adjust if sprite has padding/transparency)

		// ADD SPELLS
		// Register fireball sprite (actual size is 28x22)
		SpriteID fireSpellSpriteId = scene.GetSpriteManager().RegisterTexture(
			gResourcePath + "sprites/fireball_flying_1.png", ECSEngine::Rect(0.f, 0.f, 28.f, 22.f));

		// Create SpellComponent for player
		ECSEngine::SpellComponent spellComp;

		// Fire spell: high damage, fast, short cooldown
		auto &fireProps = spellComp.spellProperties[static_cast<size_t>(ECSEngine::SpellType::Fire)];
		fireProps.damage = 15.0f;
		fireProps.speed = 400.0f;
		fireProps.cooldown = 0.5f;
		fireProps.lifetime = 240.0f;
		fireProps.size = 28.0f;
		fireProps.spriteId = fireSpellSpriteId;
		
		// Add explosion animation frames, what does this do?
		if (fireballExplosionAnim.animations.count("explosion") > 0)
		{
			fireProps.explosionFrames = fireballExplosionAnim.animations["explosion"];
			fireProps.explosionSize = 32.0f;
		}

		// Configure element switch cooldown (time between switching elements)
		spellComp.switchCooldownDuration = 0.5f;

		// Start with Fire element selected
		spellComp.selectedSpell = ECSEngine::SpellType::Fire;

		scene.GetEntityManager().template AddComponent<ECSEngine::SpellComponent>(player, spellComp);

		// CAMERA
		//  Create camera entity that follows the player
		EntityId camera = scene.GetEntityManager().CreateEntity("camera");
		ECSEngine::CameraComponent cameraComp;
		cameraComp.position = ECSEngine::Point2D(spawnX, spawnY);
		cameraComp.scale = 1.0f;
		scene.GetEntityManager().template AddComponent<ECSEngine::CameraComponent>(camera, cameraComp);

		// SCORE
		//  Set up score display system
		//  Get the ScoreComponent attached to the player entity
		auto &scoreComp = scene.GetEntityManager().template GetComponent<ECSEngine::ScoreComponent>(player);

		// Register digit sprites (0-9) from the spritesheet
		const float tileSize = 32.f;
		const float digitX = 13.f * tileSize;	  // X position of digit column in spritesheet
		const float digitStartY = 4.f * tileSize; // Y position where digit 0 starts

		// for (int digit = 0; digit < 10; ++digit)
		// {
		// 	// Calculate Y position: digits are arranged top-to-bottom (0 at top, 9 at bottom)
		// 	float digitY = digitStartY + (9 - digit) * tileSize;
		// 	SpriteID digitSpriteId = scene.GetSpriteManager().RegisterTexture(
		// 		gResourcePath + "spritesheet-tiles-default.png",
		// 		ECSEngine::Rect(digitX, digitY, tileSize, tileSize));
		// 	scoreComp.digitSpriteIds.push_back(digitSpriteId);
		// }

		// Create 3 display entities for the score
		const float digitSize = 32.f;
		const float startX = 20.f;
		const float startY = 20.f;

		// for (int i = 0; i < 3; ++i)
		// {
		// 	// Create an entity for each digit position
		// 	EntityId digitEntity = scene.GetEntityManager().CreateEntity("score_digit_" + std::to_string(i));

		// 	// Space by digitSize
		// 	scene.GetEntityManager().template AddComponent<ECSEngine::LocationComponent>(
		// 		digitEntity,
		// 		ECSEngine::LocationComponent(ECSEngine::Point2D(startX + i * digitSize, startY)));

		// 	// Initialize sprite component with digit 0 sprite
		// 	scene.GetEntityManager().template AddComponent<ECSEngine::SpriteComponent>(
		// 		digitEntity,
		// 		{scoreComp.digitSpriteIds[0], ECSEngine::Rect(0.f, 0.f, digitSize, digitSize), false});

		// 	scoreComp.displayEntityIds.push_back(digitEntity);
		// }

		// Set up HP display system (hearts)
		// Register heart sprites
		// SpriteID heartFullSpriteId = scene.GetSpriteManager().RegisterTexture(
		// 	gResourcePath + "sprites/heart_idle_0.png", ECSEngine::Rect(0.f, 0.f, 32.f, 32.f));
		// SpriteID heartEmptySpriteId = scene.GetSpriteManager().RegisterTexture(
		// 	gResourcePath + "sprites/heart_empty_0.png", ECSEngine::Rect(0.f, 0.f, 32.f, 32.f));

		// // Create HpComponent for player
		// ECSEngine::HpComponent hpComp(3, heartFullSpriteId, heartEmptySpriteId);

		// // Create 3 heart display entities at top left (below score)
		// const float heartSize = 32.f;
		// const float heartStartX = 20.f;
		// const float heartStartY = 60.f; // Below the score display

		// for (int i = 0; i < 3; ++i)
		// {
		// 	EntityId heartEntity = scene.GetEntityManager().CreateEntity("heart_" + std::to_string(i));

		// 	scene.GetEntityManager().template AddComponent<ECSEngine::LocationComponent>(
		// 		heartEntity,
		// 		ECSEngine::LocationComponent(ECSEngine::Point2D(heartStartX + i * heartSize, heartStartY)));

		// 	// Initialize with full heart sprite (inWorldSpace = false for UI)
		// 	scene.GetEntityManager().template AddComponent<ECSEngine::SpriteComponent>(
		// 		heartEntity,
		// 		{heartFullSpriteId, ECSEngine::Rect(0.f, 0.f, heartSize, heartSize), false});

		// 	hpComp.heartDisplayEntityIds.push_back(heartEntity);
		// }

		// scene.GetEntityManager().template AddComponent<ECSEngine::HpComponent>(player, hpComp);

		// // Set up checkpoint system
		// ECSEngine::CheckpointComponent checkpointComp(ECSEngine::Point2D(spawnX, spawnY));
		// scene.GetEntityManager().template AddComponent<ECSEngine::CheckpointComponent>(player, checkpointComp);

		// Register campfire sprites for checkpoints
		// SpriteID campfireUnlitSpriteId = scene.GetSpriteManager().RegisterTexture(
		// 	gResourcePath + "sprites/campfire-sprite-unlit-1.png", ECSEngine::Rect(0.f, 0.f, 64.f, 64.f));
		// SpriteID campfireLitSpriteId = scene.GetSpriteManager().RegisterTexture(
		// 	gResourcePath + "sprites/campfire-sprite-1.png", ECSEngine::Rect(0.f, 0.f, 64.f, 64.f));

		// Create a sample campfire checkpoint (checkpoint 0) at a specific location
		// You can add more campfires at different positions for different checkpoints
		// EntityId campfire0 = scene.GetEntityManager().CreateEntity("campfire_0");
		// ECSEngine::Point2D campfire0Pos(tileW * 10, tileH * 7); // Adjust position as needed

		// scene.GetEntityManager().template AddComponent<ECSEngine::LocationComponent>(
		// 	campfire0, ECSEngine::LocationComponent(campfire0Pos));
		// scene.GetEntityManager().template AddComponent<ECSEngine::SpriteComponent>(
		// 	campfire0, {campfireUnlitSpriteId, ECSEngine::Rect(0.f, 0.f, 64.f, 64.f), true});
		// scene.GetEntityManager().template AddComponent<ECSEngine::CollisionComponent>(
		// 	campfire0, ECSEngine::CollisionComponent(ECSEngine::Rect(0.f, 0.f, 64.f, 64.f), false));
		// scene.GetEntityManager().template AddComponent<ECSEngine::CampfireComponent>(
		// 	campfire0, ECSEngine::CampfireComponent(0, campfireUnlitSpriteId, campfireLitSpriteId));

		// TODO LAYERING WITH WORLD
		// SOUNDS
		//  Register sounds TODO
		scene.GetSoundManager().RegisterSound(gResourcePath + "sounds/cat_land1.ogg", "land");
		scene.GetSoundManager().RegisterSound(gResourcePath + "sounds/cat_jump1.ogg", "jump");
		scene.GetSoundManager().RegisterSound(gResourcePath + "sounds/sfx_gem.ogg", "star_collect");
		scene.GetSoundManager().RegisterSound(gResourcePath + "sounds/Walking and Running on Grass Sound Effect [Minecraft] [TubeRipper.cc].ogg", "walk");
		scene.GetSoundManager().RegisterSound(gResourcePath + "sounds/cat_take_damage1.ogg", "take_damage");

		// scene.GetSpriteManager().SaveAtlas(gResourcePath + "atlas_debug_output.png");

		// Register spell cast sounds
		// Fire spell has two whoosh variants for variety
		scene.GetSoundManager().RegisterSound(gResourcePath + "sounds/fireball_whoosh-1.ogg", "fire_cast_1");
		scene.GetSoundManager().RegisterSound(gResourcePath + "sounds/fireball_whoosh-2.ogg", "fire_cast_2");
		// scene.GetSoundManager().RegisterSound(gResourcePath + ".ogg", "water_cast");
		// scene.GetSoundManager().RegisterSound(gResourcePath + ".ogg", "wind_cast");
		// scene.GetSoundManager().RegisterSound(gResourcePath + ".ogg", "earth_cast");

		// Register spell impact sounds
		// Fire spell has two impact variants for variety
		scene.GetSoundManager().RegisterSound(gResourcePath + "sounds/fireball_impact1.ogg", "fire_impact_1");
		scene.GetSoundManager().RegisterSound(gResourcePath + "sounds/fireball_impact2.ogg", "fire_impact_2");
		// scene.GetSoundManager().RegisterSound(gResourcePath + ".ogg", "water_impact");
		// scene.GetSoundManager().RegisterSound(gResourcePath + ".ogg", "wind_impact");
		// scene.GetSoundManager().RegisterSound(gResourcePath + ".ogg", "earth_impact");

		// // Register element selection sounds (played when switching elements)
		// scene.GetSoundManager().RegisterSound(gResourcePath + ".ogg", "fire_select");
		// scene.GetSoundManager().RegisterSound(gResourcePath + ".ogg", "water_select");
		// scene.GetSoundManager().RegisterSound(gResourcePath + ".ogg", "wind_select");
		// scene.GetSoundManager().RegisterSound(gResourcePath + ".ogg", "earth_select");

		// Register enemy sounds
		scene.GetSoundManager().RegisterSound(gResourcePath + "sounds/slime_die1.ogg", "slime_die");
		scene.GetSoundManager().RegisterSound(gResourcePath + "sounds/slime_take_dmg1.ogg", "slime_damage_1");
		scene.GetSoundManager().RegisterSound(gResourcePath + "sounds/slime_take_dmg2.ogg", "slime_damage_2");
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

	// Initialize engine using GameComponents tuple
	GameEngine engine(1024, 768, "Leo the Wizard Cat");

	// Create a scene
	auto scene = engine.MakeScene();

	// Add systems to the scene in execution order
	auto &sm = scene->GetSystemManager();
	AddSystem<ECSEngine::ProcessEventsSystem>(sm);
	AddSystem<ECSEngine::CollisionUpdateSystem>(sm);
	AddSystem<ECSEngine::InputSystem>(sm);
	AddSystem<ECSEngine::SpellSystem>(sm);
	AddSystem<ECSEngine::GravitySystem>(sm);
	AddSystem<ECSEngine::MovementSystem>(sm);
	AddSystem<ECSEngine::CollisionSystem>(sm);
	AddSystem<ECSEngine::ProjectileSystem>(sm);
	AddSystem<ECSEngine::EnemySystem>(sm);
	AddSystem<ECSEngine::HpSystem>(sm);
	AddSystem<ECSEngine::CheckpointSystem>(sm);
	AddSystem<ECSEngine::ScoreSystem>(sm);
	AddSystem<ECSEngine::CameraSystem>(sm);
	AddSystem<ECSEngine::AnimationSystem>(sm); // double check TODO
	AddSystem<ECSEngine::SpriteSystem>(sm);
	AddSystem<ECSEngine::SpawnSystem>(sm);

	// Set up parallax factors for different layers
	auto &spriteSystem = sm.template GetSystem<GameSpriteSystem>();
	spriteSystem.SetParallaxFactor(-3, 0.85f);
	spriteSystem.SetParallaxFactor(-2, 0.90f);
	spriteSystem.SetParallaxFactor(-1, 0.95f);
	spriteSystem.SetParallaxFactor(0, 0.98f);
	spriteSystem.SetParallaxFactor(1, 1.0f);
	spriteSystem.SetParallaxFactor(2, 1.1f);
	spriteSystem.SetParallaxFactor(3, 1.2f);

	// Load maps into the scene
	LoadMap("maps/cat_sky_swamp.map", *scene);
	LoadMap("maps/cat_world.map", *scene);

	// Upload atlas image to GPU texture
	scene->GetSpriteManager().FinalizeAtlas();

	sf::Image atlasImg = scene->GetSpriteManager().GetTexture().copyToImage();

	if (!atlasImg.saveToFile("atlas.png"))
	{
		std::cerr << "Failed to save atlas to file\n";
	}
	else
	{
		std::cout << "Successfully saved atlas.png file (inside build folder)!\n";
	}

	// Push scene to engine and run
	engine.PushScene(scene);
	engine.Run();
	return 0;
}