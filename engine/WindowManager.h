#pragma once

#include <unordered_map>
#include <SFML/Window.hpp>
#include <SFML/Graphics.hpp>
#include "MathUtil.h"

namespace ECSEngine
{

class WindowManager
{
public:
	WindowManager(unsigned int width, unsigned int height, const std::string &title);
	sf::RenderWindow *GetWindow() const;


	// Functions to setup the camera
	void SetCamera(const Point2D &worldPt, const Point2D &screenPt);
	// syntactic sugar for placing the given point at the center of the screen
	// equivalent to passing in screenPt as (windowWidth/2, windowHeight/2)
	void SetCamera(const Point2D &worldPt);
	void SetWorldScale(float worldUnitsPerPixel);

	// Conversion functions
	float WindowToWorldX(float x) const;
	float WorldToWindowX(float x) const;
	float WindowToWorldY(float y) const;
	float WorldToWindowY(float y) const;
	Rect WindowToWorld(const Rect &rect) const;
	Rect WorldToWindow(const Rect &rect) const;
	Point2D WorldToWindow(const Point2D &pt) const;
	Point2D WindowToWorld(const Point2D &pt) const;
private:
	// the designation [[maybe_unused]]  is used so the code
	// compiles as provided, but should be removed once your
	// code is written
	[[maybe_unused]] sf::RenderWindow *mWindow;
};

}
