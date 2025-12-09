#include "SoundManager.h"
#include <iostream>

namespace ECSEngine
{

    void SoundManager::RegisterSound(const std::string &soundPath, const std::string &soundName)
    {
        sf::SoundBuffer buffer;
        if (!buffer.loadFromFile(soundPath))
        {
            std::cerr << "Failed to load sound: " << soundPath << "\n";
            return;
        }

        mSoundBuffers[soundName] = std::move(buffer);

        sf::Sound sound(mSoundBuffers[soundName]);
        mSounds.emplace(soundName, std::move(sound));
    }

    void SoundManager::PlaySound(const std::string &soundName)
    {
        auto it = mSounds.find(soundName);
        if (it != mSounds.end())
        {
            it->second.play();
        }
        else
        {
            std::cerr << "Sound not found: " << soundName << "\n";
        }
    }

    void SoundManager::PlaySound(const std::string &soundName, float volume)
    {
        auto it = mSounds.find(soundName);
        if (it != mSounds.end())
        {
            it->second.setVolume(volume);
            it->second.play();
        }
        else
        {
            std::cerr << "Sound not found: " << soundName << "\n";
        }
    }

}
