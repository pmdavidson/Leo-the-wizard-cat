#pragma once
#include "SystemManager.h"
#include "Scene.h"
#include "SpriteComponent.h"
#include "AnimationComponent.h"

namespace ECSEngine
{

template<typename... Components>
class AnimationSystem : public System<Components...>
{
public:
    bool Run(Scene<Components...>& scene) override;
};

} // namespace ECSEngine
