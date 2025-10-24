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

std::string gResourcePath = "../assets/";

int main(int argc, char *argv[])
{
	bool debugMode = false;
	if (argc >= 3 && argv[1] == std::string("-path"))
	{
		gResourcePath = argv[2];
	}
	std::cout << "Usage: " << argv[0] << " [-path resource_path]\n";
	std::cout << "Using resource path: " << gResourcePath << "\n";
	
	// Generic entity manager testing
	if (debugMode)
	{
		ECSEngine::EntityManager<int, float, bool> e;
		//	e1.AddComponent(42);
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
	
	// ECS Engine Part 1 startup code
	ECSEngine::ECSEngine<ECSEngine::LocationComponent, ECSEngine::MovementComponent, ECSEngine::CollisionComponent, ECSEngine::SpriteComponent, ECSEngine::SpawnComponent, ECSEngine::CameraComponent, ECSEngine::CameraFollower, ECSEngine::InputComponent, ECSEngine::GravityComponent, ECSEngine::CameraShake, ECSEngine::ScoreComponent> engine(1024, 768, "Test Engine");
	
	// Write code to create all the entities needed in the game here
	
	
	// Run the game
	engine.Run();
	
	
	return 0;
}
