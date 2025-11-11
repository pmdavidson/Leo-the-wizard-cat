#include "WindowManager.h"
#include <SFML/Window/VideoMode.hpp>
#include <SFML/Graphics/RenderWindow.hpp>
#include <iostream>

namespace ECSEngine
{

	WindowManager::WindowManager(unsigned int width, unsigned int height, const std::string &title)
		: mWidth(width), mHeight(height)
	{
		mWindow = new sf::RenderWindow(sf::VideoMode({width, height}), title);
	}

	WindowManager::~WindowManager()
	{
		if (mWindow)
		{
			mWindow->close();
			delete mWindow;
			mWindow = nullptr;
		}
	}

	sf::RenderWindow *WindowManager::GetWindow() const
	{
		return mWindow;
	}

	void WindowManager::SetCamera(const Point2D &worldPt, const Point2D &screenPt)
	{
		float dx = (screenPt.x - mWidth * 0.5f) * mWorldScale;
		float dy = (screenPt.y - mHeight * 0.5f) * mWorldScale;
		mCameraCenter = {worldPt.x - dx, worldPt.y - dy};
	}

	void WindowManager::SetCamera(const Point2D &worldCenter)
	{
		mCameraCenter = worldCenter;
	}

	void WindowManager::SetWorldScale(float worldUnitsPerPixel)
	{
		mWorldScale = worldUnitsPerPixel;
	}

	float WindowManager::WindowToWorldX(float x) const
	{
		return (x - mWidth * 0.5f) * mWorldScale + mCameraCenter.x;
	}

	float WindowManager::WorldToWindowX(float x) const
	{
		return ((x - mCameraCenter.x) / mWorldScale) + mWidth * 0.5f;
	}

	float WindowManager::WindowToWorldY(float y) const
	{
		return (y - mHeight * 0.5f) * mWorldScale + mCameraCenter.y;
	}

	float WindowManager::WorldToWindowY(float y) const
	{
		return ((y - mCameraCenter.y) / mWorldScale) + mHeight * 0.5f;
	}

	Rect WindowManager::WindowToWorld(const Rect &r) const
	{
		Point2D newTopLeft(WindowToWorldX(r.topLeft.x), WindowToWorldY(r.topLeft.y));
		return Rect(newTopLeft, r.width * mWorldScale, r.height * mWorldScale);
	}

	Rect WindowManager::WorldToWindow(const Rect &r) const
	{
		Point2D newTopLeft(WorldToWindowX(r.topLeft.x), WorldToWindowY(r.topLeft.y));
		return Rect(newTopLeft, r.width / mWorldScale, r.height / mWorldScale);
	}

	Point2D WindowManager::WorldToWindow(const Point2D &p) const
	{
		return {WorldToWindowX(p.x), WorldToWindowY(p.y)};
	}

	Point2D WindowManager::WindowToWorld(const Point2D &p) const
	{
		return {WindowToWorldX(p.x), WindowToWorldY(p.y)};
	}

}
