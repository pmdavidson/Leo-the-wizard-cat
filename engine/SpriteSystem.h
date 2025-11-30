#pragma once

#include "SystemManager.h"
#include "Scene.h"
#include "SpriteComponent.h"
#include "LocationComponent.h"

namespace ECSEngine
{

	template <typename... Components>
	class SpriteSystem : public System<Components...>
	{
	public:
		bool Run(Scene<Components...> &scene) override
		{
			auto &entityManager = scene.GetEntityManager();
			auto &spriteManager = scene.GetSpriteManager();
			auto &windowManager = scene.GetWindowManager();
			sf::RenderWindow *window = scene.GetWindow();

			window->clear();

			for (auto it = entityManager.begin(); it != entityManager.end(); ++it)
			{
				if (!it->isActive())
					continue;
				EntityID entityId = it->getID();

				if (entityManager.template HasComponent<SpriteComponent>(entityId))
				{
					auto &spriteComp = entityManager.template GetComponent<SpriteComponent>(entityId);

					sf::Sprite &sprite = spriteManager.GetSprite(spriteComp.spriteId);
					Point2D drawPos;

					if (spriteComp.inWorldSpace && entityManager.template HasComponent<LocationComponent>(entityId))
					{
						auto &locationComp = entityManager.template GetComponent<LocationComponent>(entityId);
						Point2D worldPos = {
							locationComp.position.x + spriteComp.bounds.topLeft.x,
							locationComp.position.y + spriteComp.bounds.topLeft.y};
						drawPos = windowManager.WorldToWindow(worldPos);
					}
					else if (entityManager.template HasComponent<LocationComponent>(entityId))
					{
						auto &locationComp = entityManager.template GetComponent<LocationComponent>(entityId);
						drawPos = Point2D(
							locationComp.position.x + spriteComp.bounds.topLeft.x,
							locationComp.position.y + spriteComp.bounds.topLeft.y);
					}
					else
					{
						drawPos = spriteComp.bounds.topLeft;
					}

					sprite.setPosition(sf::Vector2f(drawPos.x, drawPos.y));
					window->draw(sprite);
				}
			}

			return true;
		}
	};

} // namespace ECSEngine
