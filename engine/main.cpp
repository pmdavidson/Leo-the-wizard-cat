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
	int layer = 1; // Default layer for map tiles
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

	// REGISTER NON-MAP THINGS
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

	SpriteID blueSlimeSpriteId = 0;
	bool first = true;

	// Animation components to accumulate frames BEFORE adding to entities
	ECSEngine::AnimationComponent catAnim;
	ECSEngine::AnimationComponent blueSlimeAnimComp;
	ECSEngine::AnimationComponent redSlimeAnimComp;
	ECSEngine::AnimationComponent greenSlimeAnimComp;
	ECSEngine::AnimationComponent brownSlimeAnimComp;

	// Track first sprite and bounds for each entity
	SpriteID catFirstSprite = 0;
	ECSEngine::Rect catBounds;
	bool catHasSprite = false;

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

			// Use image size
			float imgWidth = static_cast<float>(original.getSize().x);
			float imgHeight = static_cast<float>(original.getSize().y);
			ECSEngine::Rect imgRect(ECSEngine::Point2D(0, 0), imgWidth, imgHeight);

			if (name == "cat")
			{
				SpriteID spriteId = scene.GetSpriteManager().RegisterTexture(fullpath, imgRect);

				// Add frame to animation map (accumulate, don't overwrite)
				catAnim.animations[animation].push_back(spriteId);

				// Track first sprite for initial display
				if (!catHasSprite)
				{
					catFirstSprite = spriteId;
					catBounds = imgRect;
					catHasSprite = true;
				}
			}
			else if (name == "blueSlime")
			{
				SpriteID spriteId = scene.GetSpriteManager().RegisterTexture(fullpath, imgRect);

				// Add frame to animation map
				blueSlimeAnimComp.animations[animation].push_back(spriteId);

				if (first)
				{
					blueSlimeSpriteId = spriteId;
					first = false;
				}
			}
			else if (name == "redSlime")
			{
				SpriteID spriteId = scene.GetSpriteManager().RegisterTexture(fullpath, imgRect);
				redSlimeAnimComp.animations[animation].push_back(spriteId);
			}
			else if (name == "greenSlime")
			{
				SpriteID spriteId = scene.GetSpriteManager().RegisterTexture(fullpath, imgRect);
				greenSlimeAnimComp.animations[animation].push_back(spriteId);
			}
			else if (name == "brownSlime")
			{
				SpriteID spriteId = scene.GetSpriteManager().RegisterTexture(fullpath, imgRect);
				brownSlimeAnimComp.animations[animation].push_back(spriteId);
			}
			else
			{
				continue;
			}
		}
	}

	// Now add the accumulated animation components to entities ONCE
	if (catHasSprite)
	{
		// Set initial animation to idleA if available
		if (catAnim.animations.count("idleA") > 0)
		{
			catAnim.currentAnimation = "idleA";
			catAnim.playing = true;
			catAnim.looping = true;
			catAnim.frameDuration = 0.15f; // Slower animations for player (was default 0.1f)
			catFirstSprite = catAnim.animations["idleA"][0];
		}
		scene.GetEntityManager().template AddComponent<ECSEngine::SpriteComponent>(player, ECSEngine::SpriteComponent(catFirstSprite, catBounds, true, 1));
		scene.GetEntityManager().template AddComponent<ECSEngine::AnimationComponent>(player, catAnim);
	}

	// REGISTER MAP THINGS HERE
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

		// Extract layer number from filename (e.g., "_swamp_sky_-3.png" -> -3)
		// The layer is the last number before .png
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
				// Decorative elements (no collision) should be on layer 0 (behind world tiles)
				entry.layer = 0;
			}
			else
			{
				entry.hasCollision = true;
				// Ground tiles (with collision) should be on layer 1 (world tiles)
				entry.layer = 1;
			}
		}
		else
		{
			// Dictionary type 1: extract layer from filename
			std::string filename = entry.texturePath;
			size_t lastUnderscore = filename.find_last_of('_');
			size_t dotPos = filename.find_last_of('.');
			if (lastUnderscore != std::string::npos && dotPos != std::string::npos && lastUnderscore < dotPos)
			{
				std::string layerStr = filename.substr(lastUnderscore + 1, dotPos - lastUnderscore - 1);
				try
				{
					entry.layer = std::stoi(layerStr);
				}
				catch (...)
				{
					// If parsing fails, use default layer 1
					entry.layer = 1;
				}
			}
			else
			{
				entry.layer = 1;
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

			EntityId id = scene.GetEntityManager().CreateEntity("tile_" + std::string(1, tile)); // what is this

			scene.GetEntityManager().template AddComponent<ECSEngine::LocationComponent>(id, ECSEngine::LocationComponent(position));

			// Drawing Sprites
			auto spriteId = scene.GetSpriteManager().RegisterTexture(
				entry.texturePath, FromSFML(entry.sourceRect));

			ECSEngine::Rect drawBounds(0.f, 0.f,
									   static_cast<float>(entry.sourceRect.size.x),
									   static_cast<float>(entry.sourceRect.size.y));
			scene.GetEntityManager().template AddComponent<ECSEngine::SpriteComponent>(id, {spriteId, drawBounds, true, entry.layer});

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
		// Spawn position - account for map origin and align collision box bottom with tile top
		// Player collision box is 28 pixels tall, so subtract that to align bottom with tile top
		float spawnX = originX + tileW * 8;
		float spawnY = originY + tileH * 3 - 28.0f; // Subtract collision height to align bottom with tile top

		// Update all SpawnComponents with slime animations
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
					spawnComp.animations = blueSlimeAnimComp.animations;
				}
				else if (spawnComp.spawnDescription == "redSlime")
				{
					spawnComp.animations = redSlimeAnimComp.animations;
				}
				else if (spawnComp.spawnDescription == "greenSlime")
				{
					spawnComp.animations = greenSlimeAnimComp.animations;
				}
				else if (spawnComp.spawnDescription == "brownSlime")
				{
					spawnComp.animations = brownSlimeAnimComp.animations;
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
		scene.GetEntityManager().template AddComponent<ECSEngine::CollisionComponent>(player, {ECSEngine::Rect(0.f, 0.f, 25.f, 28.f), false}); // Collision box: 32x32, full sprite size (adjust if sprite has padding/transparency)

		// ADD SPELLS
		// Create SpellComponent for player
		ECSEngine::SpellComponent spellComp;

		// Fire spell: high damage, fast, short cooldown
		auto &fireProps = spellComp.spellProperties[static_cast<size_t>(ECSEngine::SpellType::Fire)];
		fireProps.damage = 15.0f;
		fireProps.speed = 400.0f;
		fireProps.cooldown = 0.5f;
		fireProps.lifetime = 240.0f;
		fireProps.size = 28.0f;

		// Register fireball flying animation frames (1-10)
		for (int i = 1; i <= 10; ++i)
		{
			SpriteID id = scene.GetSpriteManager().RegisterTexture(
				gResourcePath + "sprites/fireball_flying_" + std::to_string(i) + ".png",
				ECSEngine::Rect(0.f, 0.f, 28.f, 22.f));
			fireProps.flyingFrames.push_back(id);
		}
		fireProps.spriteId = fireProps.flyingFrames[0];

		// Register fireball explosion frames (1-7)
		for (int i = 1; i <= 7; ++i)
		{
			SpriteID id = scene.GetSpriteManager().RegisterTexture(
				gResourcePath + "sprites/fireball_explosion_" + std::to_string(i) + ".png",
				ECSEngine::Rect(0.f, 0.f, 32.f, 32.f));
			fireProps.explosionFrames.push_back(id);
		}
		fireProps.explosionSize = 32.0f;

		// Water spell: moderate damage, moderate speed (sprite is 128x128)
		auto &waterProps = spellComp.spellProperties[static_cast<size_t>(ECSEngine::SpellType::Water)];
		waterProps.damage = 12.0f;
		waterProps.speed = 350.0f;
		waterProps.cooldown = 0.6f;
		waterProps.lifetime = 200.0f;
		waterProps.size = 28.0f;

		// Register waterball flying animation frames (1-3)
		for (int i = 1; i <= 3; ++i)
		{
			SpriteID id = scene.GetSpriteManager().RegisterTexture(
				gResourcePath + "sprites/waterball_flying_" + std::to_string(i) + ".png",
				ECSEngine::Rect(0.f, 0.f, 32.f, 32.f));
			waterProps.flyingFrames.push_back(id);
		}
		waterProps.spriteId = waterProps.flyingFrames[0];

		// Register waterball explosion frames (1-5)
		for (int i = 1; i <= 5; ++i)
		{
			SpriteID id = scene.GetSpriteManager().RegisterTexture(
				gResourcePath + "sprites/waterball_explode_" + std::to_string(i) + ".png",
				ECSEngine::Rect(0.f, 0.f, 64.f, 64.f));
			waterProps.explosionFrames.push_back(id);
		}
		waterProps.explosionSize = 64.0f;

		// Rock/Earth spell: high damage, slower, longer cooldown (sprite is 64x48)
		auto &earthProps = spellComp.spellProperties[static_cast<size_t>(ECSEngine::SpellType::Earth)];
		earthProps.damage = 20.0f;
		earthProps.speed = 300.0f;
		earthProps.cooldown = 0.8f;
		earthProps.lifetime = 180.0f;
		earthProps.size = 28.0f;

		// Register rockarrow flying animation frames (1-8)
		for (int i = 1; i <= 8; ++i)
		{
			SpriteID id = scene.GetSpriteManager().RegisterTexture(
				gResourcePath + "sprites/rockarrow_flying_" + std::to_string(i) + ".png",
				ECSEngine::Rect(0.f, 0.f, 32.f, 32.f));
			earthProps.flyingFrames.push_back(id);
		}
		earthProps.spriteId = earthProps.flyingFrames[0];
		// Register rockarrow explosion frames (1-9)
		for (int i = 1; i <= 9; ++i)
		{
			SpriteID id = scene.GetSpriteManager().RegisterTexture(
				gResourcePath + "sprites/rockarrow_explode_" + std::to_string(i) + ".png",
				ECSEngine::Rect(0.f, 0.f, 64.f, 63.f));
			earthProps.explosionFrames.push_back(id);
		}
		earthProps.explosionSize = 64.0f;

		// Wind spell: low damage, very fast (placeholder - no sprites yet)
		auto &windProps = spellComp.spellProperties[static_cast<size_t>(ECSEngine::SpellType::Wind)];
		windProps.damage = 8.0f;
		windProps.speed = 500.0f;
		windProps.cooldown = 0.3f;
		windProps.lifetime = 150.0f;
		windProps.size = 24.0f;
		windProps.flyingFrames = fireProps.flyingFrames; // Use fire as placeholder
		windProps.spriteId = fireProps.spriteId;
		windProps.explosionFrames = fireProps.explosionFrames;
		windProps.explosionSize = 32.0f;

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
		cameraComp.scale = 0.65f;
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
		// Register heart sprites - load images first to get actual dimensions
		sf::Image heartFullImg;
		if (!heartFullImg.loadFromFile(gResourcePath + "sprites/heart_idle_0.png"))
		{
			std::cerr << "Failed to load heart_idle_0.png\n";
		}
		float heartWidth = static_cast<float>(heartFullImg.getSize().x);
		float heartHeight = static_cast<float>(heartFullImg.getSize().y);
		SpriteID heartFullSpriteId = scene.GetSpriteManager().RegisterTexture(
			gResourcePath + "sprites/heart_idle_0.png", ECSEngine::Rect(0.f, 0.f, heartWidth, heartHeight));
		
		sf::Image heartEmptyImg;
		if (!heartEmptyImg.loadFromFile(gResourcePath + "sprites/heart_empty_0.png"))
		{
			std::cerr << "Failed to load heart_empty_0.png\n";
		}
		float heartEmptyWidth = static_cast<float>(heartEmptyImg.getSize().x);
		float heartEmptyHeight = static_cast<float>(heartEmptyImg.getSize().y);
		SpriteID heartEmptySpriteId = scene.GetSpriteManager().RegisterTexture(
			gResourcePath + "sprites/heart_empty_0.png", ECSEngine::Rect(0.f, 0.f, heartEmptyWidth, heartEmptyHeight));

		// Create HpComponent for player (3 hearts total)
		ECSEngine::HpComponent hpComp(3, heartFullSpriteId, heartEmptySpriteId);

		// Create 5 heart display entities at top left
		// Scale hearts to be larger (1.5x size)
		const float heartScale = 1.5f;
		const float heartDisplayWidth = heartWidth * heartScale;
		const float heartDisplayHeight = heartHeight * heartScale;
		const float heartStartX = 20.f;
		const float heartStartY = 20.f; // Top left corner

		for (int i = 0; i < 5; ++i)
		{
			EntityId heartEntity = scene.GetEntityManager().CreateEntity("heart_" + std::to_string(i));

			scene.GetEntityManager().template AddComponent<ECSEngine::LocationComponent>(
				heartEntity,
				ECSEngine::LocationComponent(ECSEngine::Point2D(heartStartX + i * heartDisplayWidth, heartStartY)));

			// Initialize with full heart sprite (inWorldSpace = false for UI)
			// Use scaled size for display
			scene.GetEntityManager().template AddComponent<ECSEngine::SpriteComponent>(
				heartEntity,
				{heartFullSpriteId, ECSEngine::Rect(0.f, 0.f, heartDisplayWidth, heartDisplayHeight), false});

			hpComp.heartDisplayEntityIds.push_back(heartEntity);
		}

		scene.GetEntityManager().template AddComponent<ECSEngine::HpComponent>(player, hpComp);

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
		scene.GetSoundManager().RegisterSound(gResourcePath + "sounds/waterball_whoosh_1.ogg", "water_cast");
		scene.GetSoundManager().RegisterSound(gResourcePath + "sounds/rock_whooshing_1.ogg", "earth_cast");
		// Wind has no sound assets yet, using fire as placeholder
		scene.GetSoundManager().RegisterSound(gResourcePath + "sounds/fireball_whoosh-2.ogg", "wind_cast");

		// Register spell impact sounds
		// Fire spell has two impact variants for variety
		scene.GetSoundManager().RegisterSound(gResourcePath + "sounds/fireball_impact1.ogg", "fire_impact_1");
		scene.GetSoundManager().RegisterSound(gResourcePath + "sounds/fireball_impact2.ogg", "fire_impact_2");
		scene.GetSoundManager().RegisterSound(gResourcePath + "sounds/waterball_explode_1.ogg", "water_impact");
		scene.GetSoundManager().RegisterSound(gResourcePath + "sounds/rock_explode_1.ogg", "earth_impact");
		// Wind has no sound assets yet, using fire as placeholder
		scene.GetSoundManager().RegisterSound(gResourcePath + "sounds/fireball_impact1.ogg", "wind_impact");

		// Register element selection sounds (using cast sounds as placeholders)
		scene.GetSoundManager().RegisterSound(gResourcePath + "sounds/fireball_whoosh-1.ogg", "fire_select");
		scene.GetSoundManager().RegisterSound(gResourcePath + "sounds/waterball_whoosh_1.ogg", "water_select");
		scene.GetSoundManager().RegisterSound(gResourcePath + "sounds/rock_whooshing_1.ogg", "earth_select");
		// Wind has no sound assets yet, using fire as placeholder
		scene.GetSoundManager().RegisterSound(gResourcePath + "sounds/fireball_whoosh-2.ogg", "wind_select");

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
	spriteSystem.SetParallaxFactor(0, 1.0f);  // World tiles (decorative) - no parallax
	spriteSystem.SetParallaxFactor(1, 1.0f);  // World tiles (ground) - no parallax
	spriteSystem.SetParallaxFactor(2, 1.0f); // Props/enemies/spells - align with collisions
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