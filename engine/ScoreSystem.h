#pragma once

#include "SystemManager.h"
#include "Scene.h"
#include "ScoreComponent.h"
#include "SpriteComponent.h"

namespace ECSEngine
{

	template <typename... Components>
	class ScoreSystem : public System<Components...>
	{
	public:
		bool Run(Scene<Components...> &scene) override
		{
			auto &entityManager = scene.GetEntityManager();

			for (auto it = entityManager.begin(); it != entityManager.end(); ++it)
			{
				if (!it->isActive())
					continue;
				EntityID entityId = it->getID();

				if (entityManager.template HasComponent<ScoreComponent>(entityId))
				{
					auto &scoreComp = entityManager.template GetComponent<ScoreComponent>(entityId);

					std::string scoreStr = std::to_string(scoreComp.currentScore);
					if (scoreStr.length() < 3)
						scoreStr = std::string(3 - scoreStr.length(), '0') + scoreStr;
					if (scoreStr.length() > 3)
						scoreStr = scoreStr.substr(scoreStr.length() - 3);

					for (size_t i = 0; i < scoreComp.displayEntityIds.size() && i < 3; ++i)
					{
						EntityID displayEntityId = scoreComp.displayEntityIds[i];
						if (entityManager.template HasComponent<SpriteComponent>(displayEntityId))
						{
							int digit = scoreStr[i] - '0';
							if (digit >= 0 && digit < 10 && digit < static_cast<int>(scoreComp.digitSpriteIds.size()))
							{
								auto &spriteComp = entityManager.template GetComponent<SpriteComponent>(displayEntityId);
								spriteComp.spriteId = scoreComp.digitSpriteIds[digit];
							}
						}
					}
				}
			}

			return true;
		}
	};

} // namespace ECSEngine