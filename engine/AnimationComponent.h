#pragma once
#include <unordered_map>
#include <vector>
#include <string>
#include "SpriteManager.h"

namespace ECSEngine
{

    struct AnimationComponent
    {
        // Stores animations by name (e.g., "idle", "hurt")
        std::unordered_map<std::string, std::vector<SpriteID>> animations;

        std::string currentAnimation;     // Active animation name
        float frameDuration = 0.1f;

        size_t currentFrame = 0;
        float frameTimer = 0.0f;

        bool looping = true;
        bool playing = false;

        AnimationComponent() = default;

        void AddAnimation(const std::string& name, const SpriteID& frame)
        {
            animations[name].emplace_back(frame);
        }

        void Play(const std::string& name, bool loop = true)
        {
            // If switching animations, reset frame state
            if (currentAnimation != name)
            {
                currentAnimation = name;
                currentFrame = 0;
                frameTimer = 0.0f;
            }

            looping = loop;
            playing = true;
        }

        void Stop()
        {
            playing = false;
            currentFrame = 0;
            frameTimer = 0.0f;
        }

        const std::vector<SpriteID>* GetCurrentFrames() const
        {
            auto it = animations.find(currentAnimation);
            return (it != animations.end()) ? &it->second : nullptr;
        }
    };

} // namespace ECSEngine
