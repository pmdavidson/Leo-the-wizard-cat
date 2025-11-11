#pragma once

#include <unordered_map>
#include <SFML/Audio.hpp>
#include <string>

namespace ECSEngine {

	class SoundManager {
	public:
		SoundManager() = default;
		~SoundManager() = default;

		void RegisterSound(const std::string& soundPath, const std::string& soundName);
		void PlaySound(const std::string& soundName);

		// Optional: Set volume for a specific sound (0–100)
		// void SetVolume(const std::string& soundName, float volume);

	private:
		std::unordered_map<std::string, sf::SoundBuffer> mSoundBuffers;
		std::unordered_map<std::string, sf::Sound> mSounds;
	};

}
