#pragma once

#include <memory>
#include <SFML/Graphics.hpp>
#include <SFML/System.hpp>
#include "EntityManager.h"
#include "SpriteManager.h"
#include "SoundManager.h"
#include "WindowManager.h"
#include "ShaderManager.h"
#include "SystemManager.h"

namespace ECSEngine
{

	template <typename... Components>
	class ECSEngine;

	/**
	 * @brief Represents a scene in the game with its own managers and systems.
	 *
	 * A Scene contains all the managers needed for game logic
	 *
	 * @tparam Components The component types that can be attached to entities.
	 */
	template <typename... Components>
	class Scene
	{
	public:
		/**
		 * @brief Constructs a Scene with a shared pointer to the window.
		 *
		 * @param window Shared pointer to the render window.
		 */
		Scene(std::shared_ptr<sf::RenderWindow> window)
			: mWindow(window), mWindowManager(window.get(), window->getSize().x, window->getSize().y), mTotalTime(sf::Time::Zero)
		{
		}

		/**
		 * @brief Runs the scene by executing all systems.
		 *
		 * @param engine Reference to the ECS engine managing this scene.
		 */
		void Run(ECSEngine<Components...> &engine)
		{
			mDeltaTime = mClock.restart();   // update deltaTime
			mTotalTime += mDeltaTime;        // accumulate total time

			mSystemManager.Run(*this);       // run systems with correct dt
		}


		/**
		 * @brief Gets a reference to the SoundManager.
		 *
		 * @return SoundManager& Reference to the SoundManager instance.
		 */
		SoundManager &GetSoundManager()
		{
			return mSoundManager;
		}

		/**
		 * @brief Gets a reference to the SpriteManager.
		 *
		 * @return SpriteManager& Reference to the SpriteManager instance.
		 */
		SpriteManager &GetSpriteManager()
		{
			return mSpriteManager;
		}

		/**
		 * @brief Gets a reference to the EntityManager.
		 *
		 * @return EntityManager<Components...>& Reference to the EntityManager instance.
		 */
		EntityManager<Components...> &GetEntityManager()
		{
			return mEntityManager;
		}

		/**
		 * @brief Gets a reference to the SystemManager.
		 *
		 * @return SystemManager<Components...>& Reference to the SystemManager instance.
		 */
		SystemManager<Components...> &GetSystemManager()
		{
			return mSystemManager;
		}

		/**
		 * @brief Gets a reference to the WindowManager.
		 *
		 * @return WindowManager& Reference to the WindowManager instance.
		 */
		WindowManager &GetWindowManager()
		{
			return mWindowManager;
		}

		/**
		 * @brief Gets a reference to the ShaderManager.
		 *
		 * @return ShaderManager& Reference to the ShaderManager instance.
		 */
		ShaderManager &GetShaderManager()
		{
			return mShaderManager;
		}

		/**
		 * @brief Gets the total time the scene has been running.
		 *
		 * @return sf::Time Total elapsed time since scene creation.
		 */
		sf::Time GetTotalTime() const
		{
			return mTotalTime;
		}

		float GetDeltaSeconds()
		{
			sf::Time dt = mClock.restart();
			return dt.asSeconds();
		}

		/**
		 * @brief Marks the scene as complete.
		 *
		 * When a scene is marked as complete, it will be popped from the scene stack.
		 */
		void SetComplete()
		{
			mIsComplete = true;
		}

		/**
		 * @brief Checks if the scene is complete.
		 *
		 * @return true if the scene is marked as complete, false otherwise.
		 */
		bool IsComplete() const
		{
			return mIsComplete;
		}

		/**
		 * @brief Gets a pointer to the render window.
		 *
		 * @return sf::RenderWindow* Pointer to the render window.
		 */
		sf::RenderWindow *GetWindow() const
		{
			return mWindow.get();
		}

	private:
		std::shared_ptr<sf::RenderWindow> mWindow;
		EntityManager<Components...> mEntityManager;
		SpriteManager mSpriteManager;
		SoundManager mSoundManager;
		WindowManager mWindowManager;
		ShaderManager mShaderManager;
		SystemManager<Components...> mSystemManager;

		sf::Clock mClock;
		sf::Time mDeltaTime = sf::Time::Zero;
		sf::Time mTotalTime;
		bool mIsComplete = false;
	};

}
