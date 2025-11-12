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

		InputComponent()
		{
			keydown.reset(); // sets all bits to 0 (no keys pressed)
		}
	};
}