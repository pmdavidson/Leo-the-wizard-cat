#pragma once

#include <unordered_map>
#include "MathUtil.h"
#include <bitset>

namespace ECSEngine
{

	struct LocationComponent
	{
		Point2D position; // Base location of the entity
	};

	struct MovementComponent
	{
		Point2D velocity; // Velocity of the entity
	};

	struct GravityComponent
	{
		Point2D acceleration; // Acceleration acting on the entity each frame
	};

}
