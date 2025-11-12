#pragma once

#include <unordered_map>
#include <SFML/Window/Keyboard.hpp>
#include <bitset>
#include "MathUtil.h"

namespace ECSEngine
{
	struct InputComponent
	{
		std::bitset<sf::Keyboard::ScancodeCount> keydown;
<<<<<<< HEAD
=======

		InputComponent()
        {
            keydown.reset(); // sets all bits to 0 (no keys pressed)
        }
	};
>>>>>>> 22229c09a98d4001081ea029ad2991fdb6fdc2a9

		InputComponent()
		{
			keydown.reset(); // sets all bits to 0 (no keys pressed)
		}
	};
}