#pragma once

#include <SFML/Graphics.hpp>
#include <SFML/Window.hpp>
#include <vector>
#include "MathUtil.h"
#include "EntityManager.h"
#include "SpriteManager.h"
#include "SoundManager.h"
#include "CameraComponent.h"
#include "InputComponent.h"
#include "LocationComponent.h"
#include "CollisionComponent.h"
#include "SpriteComponent.h"
#include "WindowManager.h"
#include "SpawnComponent.h"
#include "ScoreComponent.h"

namespace ECSEngine
{

template <typename... Components>
class ECSEngine
{
public:
	ECSEngine(unsigned int width, unsigned int height, const std::string& name);
	
	void Run();
	
	SoundManager &GetSoundManager()
	{
		return mSoundManager;
	}

	SpriteManager &GetSpriteManager()
	{
		return mSpriteManager;
	}
	
	EntityManager<Components...> &GetEntityManager()
	{
		return mEntityManager;
	}
	
private:
	EntityManager<Components...> mEntityManager;
	SpriteManager mSpriteManager;
	SoundManager mSoundManager;
	WindowManager mWindowManager;
};


template <typename... Components>
ECSEngine<Components...>::ECSEngine(unsigned int width, unsigned int height, const std::string& name)
:mWindowManager(width, height, name)
{
}

template <typename... Components>
void ECSEngine<Components...>::Run()
{
	// while window is still open
	while (true)
	{
		// All your systems go here
	}
}


}

