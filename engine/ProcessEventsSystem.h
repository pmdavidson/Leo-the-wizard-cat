#pragma once

#include "SystemManager.h"
#include "InputComponent.h"
#include "Scene.h"
#include <SFML/Window.hpp>

namespace ECSEngine
{

	template <typename... Components>
	class ProcessEventsSystem : public System<Components...>
	{
	public:
		bool Run(Scene<Components...> &scene) override
		{
			sf::RenderWindow *window = scene.GetWindow();
			auto &entityManager = scene.GetEntityManager();

			// Process all pending events
			while (auto event = window->pollEvent())
			{
				// Handle window close event
				if (event->is<sf::Event::Closed>())
				{
					window->close();
					continue;
				}
				else if (auto keyEvent = event->getIf<sf::Event::KeyPressed>())
				{
					// Convert Key to Scancode for InputComponent
					sf::Keyboard::Scancode scancode = sf::Keyboard::delocalize(keyEvent->code);

					// Only process valid scancodes
					if (scancode != sf::Keyboard::Scan::Unknown)
					{
						size_t scancodeIndex = static_cast<size_t>(scancode);
						// Check that scancode is within bitset range
						if (scancodeIndex < sf::Keyboard::ScancodeCount)
						{
							// Update input components for all entities with InputComponent
							for (auto it = entityManager.begin(); it != entityManager.end(); ++it)
							{
								if (!it->isActive())
									continue;
								EntityID entityId = it->getID();

								if (entityManager.template HasComponent<InputComponent>(entityId))
								{
									auto &inputComp = entityManager.template GetComponent<InputComponent>(entityId);
									inputComp.keydown.set(scancodeIndex);
								}
							}
						}
					}
				}
				else if (auto keyEvent = event->getIf<sf::Event::KeyReleased>())
				{
					// Convert Key to Scancode for InputComponent
					sf::Keyboard::Scancode scancode = sf::Keyboard::delocalize(keyEvent->code);

					// Only process valid scancodes
					if (scancode != sf::Keyboard::Scan::Unknown)
					{
						size_t scancodeIndex = static_cast<size_t>(scancode);
						// Bounds check to ensure scancode is within bitset range
						if (scancodeIndex < sf::Keyboard::ScancodeCount)
						{
							// Clear key when released
							for (auto it = entityManager.begin(); it != entityManager.end(); ++it)
							{
								if (!it->isActive())
									continue;
								EntityID entityId = it->getID();

								if (entityManager.template HasComponent<InputComponent>(entityId))
								{
									auto &inputComp = entityManager.template GetComponent<InputComponent>(entityId);
									inputComp.keydown.reset(scancodeIndex);
								}
							}
						}
					}
				}
			}

			return true;
		}
	};

} // namespace ECSEngine
