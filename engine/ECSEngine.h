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

	/**
	 * @brief Resolves an AABB collision between two rectangles.
	 *
	 * @param a The first rectangle (will be modified to resolve collision)
	 * @param b The second rectangle (static reference)
	 * @param aPrev The previous position of rectangle a
	 * @param bPrev The previous position of rectangle b
	 * @param flagsA Collision flags for rectangle a
	 * @param flagsB Collision flags for rectangle b
	 * @param velocityA Optional velocity of rectangle a for collision direction determination
	 */
	inline void ResolveAABBCollision(Rect &a, const Rect &b,
									 const Rect &aPrev, const Rect &bPrev,
									 CollisionFlags &flagsA, CollisionFlags &flagsB,
									 const Point2D &velocityA = Point2D(0, 0))
	{
		Point2D overlap = GetOverlap(a, b);
		if (overlap.x <= 0.0f || overlap.y <= 0.0f)
			return;

		// Small delta to add separation
		const float delta = 0.01f;

		// Compute center delta
		Point2D centerA = aPrev.topLeft + Point2D(aPrev.width / 2, aPrev.height / 2);
		Point2D centerB = bPrev.topLeft + Point2D(bPrev.width / 2, bPrev.height / 2);
		float dx = centerA.x - centerB.x;
		float dy = centerA.y - centerB.y;

		// Determine which axis to resolve
		bool resolveX = overlap.x < overlap.y;

		if (resolveX)
		{
			if (dx < 0.0f && velocityA.x >= 0.0f)
			{
				// Came from left, hit right side of B
				a.topLeft.x -= overlap.x + delta;
				flagsA.right = true;
				flagsB.left = true;
			}
			else if (dx > 0.0f && velocityA.x <= 0.0f)
			{
				// Came from right, hit left side of B
				a.topLeft.x += overlap.x + delta;
				flagsA.left = true;
				flagsB.right = true;
			}
		}
		else
		{
			if (dy < 0.0f && velocityA.y >= 0.0f)
			{
				// Came from above, landed on B
				a.topLeft.y -= overlap.y + delta;
				flagsA.bottom = true;
				flagsB.top = true;
			}
			else if (dy > 0.0f && velocityA.y <= 0.0f)
			{
				// Came from below, hit ceiling
				a.topLeft.y += overlap.y + delta;
				flagsA.top = true;
				flagsB.bottom = true;
			}
		}
	}

}
