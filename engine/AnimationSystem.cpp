#include "AnimationSystem.h"

namespace ECSEngine
{

template<typename... Components>
bool AnimationSystem<Components...>::Run(Scene<Components...>& scene)
{
    auto& entityManager = scene.GetEntityManager();

    float dt = scene.GetDeltaTime();  // or however your engine gives delta time

    for (auto it = entityManager.begin(); it != entityManager.end(); ++it)
    {
        if (!it->isActive()) continue;

        EntityID id = it->getID();

        // Must have sprite + animation component
        if (!entityManager.template HasComponent<SpriteComponent>(id) ||
            !entityManager.template HasComponent<AnimationComponent>(id))
            continue;

        auto& sprite = entityManager.template GetComponent<SpriteComponent>(id);
        auto& anim   = entityManager.template GetComponent<AnimationComponent>(id);

        if (anim.frames.empty())
            continue;

        // Advance the animation timer
        anim.frameTimer += dt;

        if (anim.frameTimer >= anim.frameDuration)
        {
            anim.frameTimer -= anim.frameDuration;
            anim.currentFrame++;

            if (anim.currentFrame >= anim.frames.size())
            {
                if (anim.looping)
                    anim.currentFrame = 0;
                else
                    anim.currentFrame = anim.frames.size() - 1;
            }

            sprite.spriteId = anim.frames[anim.currentFrame];
        }
    }

    return true;
}

} // namespace ECSEngine
