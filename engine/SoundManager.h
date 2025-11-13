#pragma once

#include <unordered_map>
#include <SFML/Audio.hpp>
#include <string>

namespace ECSEngine
{

	/**
	 * @brief Manages sound resources and audio playback for the ECS engine.
	 *
	 * @section Lifetime and Validity
	 *
	 * Sound buffers and sound instances are stored in unordered_maps and remain
	 * valid for the SoundManager's lifetime. Sound names remain valid identifiers
	 * for the SoundManager's lifetime. PlaySound() allows multiple sounds to play simultaneously.
	 */

	class SoundManager
	{
	public:
		SoundManager() = default;
		~SoundManager() = default;

		/**
		 * @brief Registers a sound file and creates a sound instance for playback.
		 *
		 * @param soundPath Path to the sound file to load.
		 * @param soundName Unique name identifier for the sound. Must be unique.
		 *
		 * @note Sound buffer and instance remain valid for the SoundManager's lifetime.
		 * @warning Registering a sound with an existing name will overwrite the previous sound.
		 */
		void RegisterSound(const std::string &soundPath, const std::string &soundName);

		/**
		 * @brief Plays a registered sound by name.
		 *
		 * @param soundName The name identifier of the sound to play.
		 *
		 * @note Playback is non-blocking. Multiple sounds can play simultaneously.
		 * @warning The sound must be registered before it can be played.
		 */
		void PlaySound(const std::string &soundName);

	private:
		std::unordered_map<std::string, sf::SoundBuffer> mSoundBuffers;
		std::unordered_map<std::string, sf::Sound> mSounds;
	};

}
