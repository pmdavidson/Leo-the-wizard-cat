#pragma once

#include <unordered_map>
#include <SFML/Graphics/Sprite.hpp>
#include <SFML/Graphics/Texture.hpp>
#include "MathUtil.h"
#include <bitset>

namespace ECSEngine
{

	enum class CollisionSide
	{
		Top = 0,
		Bottom = 1,
		Left = 2,
		Right = 3
	};

	struct CollisionComponent
	{
		std::bitset<4> collisionSides; // Tracks which sides were in collision (Top, Bottom, Left, Right)
		Rect currentBoundingBox;	   // Current frame's bounding box
		Rect previousBoundingBox;	   // Previous frame's bounding box
		bool isStatic;				   // Whether the entity is static or dynamic
	};

}
