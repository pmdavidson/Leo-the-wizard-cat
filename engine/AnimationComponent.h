#pragma once
#include <vector>
#include "SpriteManager.h"

namespace ECSEngine
{

struct AnimationComponent
{
    std::vector<SpriteID> frames;
    float frameDuration = 0.1f;

    size_t currentFrame = 0;
    float frameTimer = 0.0f;

    bool looping = true;

    AnimationComponent() = default;

    AnimationComponent(const std::vector<SpriteID>& frames,
                       float frameDuration,
                       bool looping = true)
        : frames(frames),
          frameDuration(frameDuration),
          looping(looping)
    {
    }
};

} // namespace ECSEngine
