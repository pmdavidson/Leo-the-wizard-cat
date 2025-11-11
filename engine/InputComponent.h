#pragma once

#include <unordered_map>
#include <SFML/Window/Keyboard.hpp>

#include "MathUtil.h"
#include <bitset>

namespace ECSEngine
{

	struct InputComponent
	{
		std::bitset<sf::Keyboard::ScancodeCount> keydown;
	};

}
