#pragma once

#include <unordered_map>
#include <SFML/Window.hpp>
#include <SFML/Graphics.hpp>
#include "MathUtil.h"

namespace ECSEngine
{

	/**
	 * @brief Manages the render window and camera transformations for the engine.
	 *
	 * @section Lifetime and Validity
	 *
	 * GetWindow() returns a pointer valid for the WindowManager's lifetime. The
	 * window is created in the constructor and destroyed in the destructor. Camera
	 * settings persist until explicitly changed. Coordinate transformation functions
	 * return values independent of the WindowManager's lifetime, but depend on
	 * current camera settings.
	 */
	class WindowManager
	{
	public:
		/**
		 * @brief Constructs a WindowManager and creates the render window.
		 *
		 * @param width Width of the window in pixels.
		 * @param height Height of the window in pixels.
		 * @param title Title text displayed in the window's title bar.
		 */
		WindowManager(unsigned int width, unsigned int height, const std::string &title);

		/**
		 * @brief Constructs a WindowManager using an existing window.
		 *
		 * @param window Pointer to an existing render window.
		 * @param width Width of the window in pixels.
		 * @param height Height of the window in pixels.
		 *
		 * @note The WindowManager will not delete the window in its destructor.
		 */
		WindowManager(sf::RenderWindow *window, unsigned int width, unsigned int height);

		/**
		 * @brief Destroys the WindowManager and closes the render window if owned.
		 */
		~WindowManager();

		/**
		 * @brief Gets a pointer to the managed render window.
		 *
		 * @return sf::RenderWindow* Pointer valid for the WindowManager's lifetime.
		 *
		 * @warning Do not store this pointer beyond the WindowManager's lifetime.
		 */
		sf::RenderWindow *GetWindow() const;

		/**
		 * @brief Sets the camera position by mapping a world point to a screen point.
		 *
		 * @param worldPt The world space point to position.
		 * @param screenPt The screen space point where worldPt should appear.
		 *
		 * @note Camera settings persist until explicitly changed.
		 */
		void SetCamera(const Point2D &worldPt, const Point2D &screenPt);

		/**
		 * @brief Sets the camera to center on a world point.
		 *
		 * @param worldPt The world space point to center the camera on.
		 *
		 * @note Camera settings persist until explicitly changed.
		 */
		void SetCamera(const Point2D &worldPt);

		/**
		 * @brief Sets the world scale (world units per pixel).
		 *
		 * @param worldUnitsPerPixel The number of world units per pixel.
		 *
		 * @note Default scale is 1.0. Camera settings persist until explicitly changed.
		 */
		void SetWorldScale(float worldUnitsPerPixel);

		/**
		 * @brief Converts a window X coordinate to world X coordinate.
		 *
		 * @param x Window X coordinate in pixels.
		 * @return float World X coordinate in world units.
		 */
		float WindowToWorldX(float x) const;

		/**
		 * @brief Converts a world X coordinate to window X coordinate.
		 *
		 * @param x World X coordinate in world units.
		 * @return float Window X coordinate in pixels.
		 */
		float WorldToWindowX(float x) const;

		/**
		 * @brief Converts a window Y coordinate to world Y coordinate.
		 *
		 * @param y Window Y coordinate in pixels.
		 * @return float World Y coordinate in world units.
		 */
		float WindowToWorldY(float y) const;

		/**
		 * @brief Converts a world Y coordinate to window Y coordinate.
		 *
		 * @param y World Y coordinate in world units.
		 * @return float Window Y coordinate in pixels.
		 */
		float WorldToWindowY(float y) const;

		/**
		 * @brief Converts a window rectangle to world rectangle.
		 *
		 * @param rect Rectangle in window pixel coordinates.
		 * @return Rect Rectangle in world units.
		 */
		Rect WindowToWorld(const Rect &rect) const;

		/**
		 * @brief Converts a world rectangle to window rectangle.
		 *
		 * @param rect Rectangle in world units.
		 * @return Rect Rectangle in window pixel coordinates.
		 */
		Rect WorldToWindow(const Rect &rect) const;

		/**
		 * @brief Converts a world point to window point.
		 *
		 * @param pt Point in world units.
		 * @return Point2D Point in window pixel coordinates.
		 */
		Point2D WorldToWindow(const Point2D &pt) const;

		/**
		 * @brief Converts a window point to world point.
		 *
		 * @param pt Point in window pixel coordinates.
		 * @return Point2D Point in world units.
		 */
		Point2D WindowToWorld(const Point2D &pt) const;

	private:
		sf::RenderWindow *mWindow;
		unsigned int mWidth, mHeight;
		Point2D mCameraCenter{0.f, 0.f};
		float mWorldScale = 1.0f; // world units per pixel
		bool mOwnsWindow = true;  // Whether this WindowManager owns the window
	};

}
