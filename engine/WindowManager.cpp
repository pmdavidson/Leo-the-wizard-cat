#include "WindowManager.h"

namespace ECSEngine
{

WindowManager::WindowManager(unsigned int width, unsigned int height, const std::string &title)
{
	
}

sf::RenderWindow *WindowManager::GetWindow() const
{
	return 0; // replace this code
}


// Functions to setup the camera
void WindowManager::SetCamera(const Point2D &worldPt, const Point2D &screenPt)
{
	
}

// syntactic sugar for placing the given point at the center of the screen
// equivalent to passing in screenPt as (windowWidth/2, windowHeight/2)
void WindowManager::SetCamera(const Point2D &worldPt)
{
	
}

void WindowManager::SetWorldScale(float worldUnitsPerPixel)
{
	
}


// Conversion functions
float WindowManager::WindowToWorldX(float x) const
{
	return 0;
}

float WindowManager::WorldToWindowX(float x) const
{
	return 0;
}

float WindowManager::WindowToWorldY(float y) const
{
	return 0;
}

float WindowManager::WorldToWindowY(float y) const
{
	return 0;
}

Rect WindowManager::WindowToWorld(const Rect &rect) const
{
	return rect;
}

Rect WindowManager::WorldToWindow(const Rect &rect) const
{
	return rect;
}

Point2D WindowManager::WorldToWindow(const Point2D &pt) const
{
	return pt;
}

Point2D WindowManager::WindowToWorld(const Point2D &pt) const
{
	return pt;
}


}
