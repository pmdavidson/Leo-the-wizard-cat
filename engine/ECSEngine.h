#pragma once

#include <SFML/Graphics.hpp>
#include <SFML/Window.hpp>
#include <vector>
#include <string>
#include <memory>
#include "Scene.h"
#include "MathUtil.h"
#include "CollisionComponent.h"

static_assert(std::is_same_v<sf::IntRect, sf::Rect<int>>, "IntRect is not defined correctly.");

namespace ECSEngine
{

	// Main ECS engine class that manages scenes and the render window.
	template <typename... Components>
	class ECSEngine
	{
	public:
		// Constructs the ECS engine and initializes the window.
		ECSEngine(unsigned int width, unsigned int height, const std::string &name)
			: mWidth(width), mHeight(height), mWindowName(name)
		{
			mWindow = std::make_shared<sf::RenderWindow>(
				sf::VideoMode({width, height}), name);
		}

		//	Destructor for ECSEngine.
		~ECSEngine()
		{
			// Clear all scenes before destroying the window
			mScenes.clear();
		}

		/**
		 * @brief Deleted copy constructor.
		 */
		ECSEngine(ECSEngine const &) = delete;

		/**
		 * @brief Runs the main game loop until all scenes are complete or window is closed.
		 *
		 * Continuously runs the top scene in the scene stack. When a scene completes,
		 * it is popped from the stack. The loop continues until all scenes are complete
		 * or the window is closed. The engine handles the final display call on the window.
		 */
		void Run()
		{
			sf::Clock clock;
			sf::Time frameTime = sf::seconds(1.0f / 60.0f); // Target 60 FPS

			while (mWindow->isOpen() && !mScenes.empty())
			{
				// Run the top scene
				auto &topScene = mScenes.back();
				topScene->Run(*this);

				// Check if the scene is complete and pop it if so
				if (topScene->IsComplete())
				{
					mScenes.pop_back();
				}

				// Display the window (final display call)
				mWindow->display();

				// Frame rate limiting
				sf::Time elapsed = clock.restart();
				if (elapsed < frameTime)
				{
					sf::sleep(frameTime - elapsed);
				}
			}
		}

		/**
		 * @brief Creates a new scene and returns a shared pointer to it.
		 *
		 * @return std::shared_ptr<Scene<Components...>> Shared pointer to the newly created scene.
		 */
		std::shared_ptr<Scene<Components...>> MakeScene()
		{
			return std::make_shared<Scene<Components...>>(mWindow);
		}

		/**
		 * @brief Lets us get the shared render window pointer for custom scenes.
		 */
		std::shared_ptr<sf::RenderWindow> GetWindowPtr() const
		{
			return mWindow;
		}

		/**
		 * @brief Pushes a scene onto the scene stack.
		 *
		 * @param scene Shared pointer to the scene to push.
		 */
		void PushScene(std::shared_ptr<Scene<Components...>> scene)
		{
			mScenes.push_back(scene);
		}

		/**
		 * @brief Pops the top scene from the scene stack.
		 */
		void PopScene()
		{
			if (!mScenes.empty())
			{
				mScenes.pop_back();
			}
		}

	private:
		std::shared_ptr<sf::RenderWindow> mWindow;
		std::vector<std::shared_ptr<Scene<Components...>>> mScenes;
		unsigned int mWidth;
		unsigned int mHeight;
		std::string mWindowName;
	};
}
