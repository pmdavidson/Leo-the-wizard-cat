#pragma once

#include "SystemManager.h"
#include "Scene.h"
#include "SpriteComponent.h"
#include "AnimationComponent.h"

namespace ECSEngine
{

template <typename... Components>
class AnimationSystem : public System<Components...>
{
public:
    bool Run(Scene<Components...> &scene) override
    {
        auto &entityManager = scene.GetEntityManager();
        float dt = 1.0f / 240.0f;

        for (auto it = entityManager.begin(); it != entityManager.end(); ++it)
        {
            if (!it->isActive())
                continue;

            EntityID id = it->getID();

            if (!entityManager.template HasComponent<SpriteComponent>(id) ||
                !entityManager.template HasComponent<AnimationComponent>(id))
                continue;

            auto &sprite = entityManager.template GetComponent<SpriteComponent>(id);
            auto &anim = entityManager.template GetComponent<AnimationComponent>(id);

            if (!anim.playing)
                continue;

            const std::vector<SpriteID> *frames = anim.GetCurrentFrames();
            if (!frames || frames->empty())
                continue;

            anim.frameTimer += dt;
                
            // checks if a current animation is now expired after a frame
            if (anim.frameTimer >= anim.frameDuration)
            {
                //if it is, then tell it that its been a frame passed
                anim.frameTimer -= anim.frameDuration;

                //do this so it starts to go to the next animation
                anim.currentFrame++;

                //if its the last animation sprite, go back to the first animation and loop back
                if (anim.currentFrame >= frames->size())
                {
                    //checks if it wants to be looped, if it does then itll loop again
                    if (anim.looping)
                        anim.currentFrame = 0;
                    else
                        anim.currentFrame = frames->size() - 1;
                }

                //makes sprite id updated so its drawn accurately in the next frame
                sprite.spriteId = (*frames)[anim.currentFrame];
            }
        }

        return true;
    }
};

}