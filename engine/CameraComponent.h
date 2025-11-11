#pragma once

#include "MathUtil.h"
#include "Entity.h"

namespace ECSEngine
{

	struct CameraComponent
	{
		Point2D position; // Position of the camera in world coordinates
		float scale;	  // Scale of the camera in world coordinates
	};

	struct CameraFollower
	{
		EntityID entityId; // Entity id being tracked by the camera
	};

	struct CameraShake
	{
		int framesRemaining; // How many frames the shake should last
		Point2D magnitude;	 // Magnitude of the shake in each direction (x, y)
	};

}
