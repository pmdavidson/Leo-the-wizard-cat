#pragma once

#include <unordered_map>
#include <SFML/Audio.hpp>

namespace ECSEngine
{

class SoundManager
{
public:
	void RegisterSound(const std::string &soundPath, const std::string &soundName);
	void PlaySound(const std::string &soundName);
};

}
