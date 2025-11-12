#pragma once

#include <unordered_map>
#include "MathUtil.h"
#include "SpriteManager.h"
#include "Entity.h"
#include <vector>

namespace ECSEngine
{

	struct ScoreComponent
	{
		int currentScore;						// Current score of the game
		std::vector<EntityID> displayEntityIds; // Entity ids used to display the score
		std::vector<SpriteID> digitSpriteIds;	// Sprite ids corresponding to numbers 0-9 (index 0-9)
<<<<<<< HEAD
=======
	
		ScoreComponent(): currentScore(0) {}
	};
	
>>>>>>> 22229c09a98d4001081ea029ad2991fdb6fdc2a9

		ScoreComponent() : currentScore(0) {}
	};
}
