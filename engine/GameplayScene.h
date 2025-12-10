#pragma once
#pragma once
#include "Scene.h"
#include "PauseScene.h"
#include <SFML/Window/Keyboard.hpp>
#include <memory>

template <typename... Components>
class GameplayScene : public ECSEngine::Scene<Components...> {
public:
	using BaseScene = ECSEngine::Scene<Components...>;

	explicit GameplayScene(std::shared_ptr<sf::RenderWindow> window)
		: BaseScene(window)
	{
	}

	void Run(ECSEngine::ECSEngine<Components...> &engine) override
	{
		// Toggle pause on Escape key (edge-triggered)
		bool pressed = sf::Keyboard::isKeyPressed(sf::Keyboard::Scancode::Escape);
		if (pressed && !mPrevPausePressed)
		{
			auto pauseScene = std::make_shared<PauseScene<Components...>>(this->GetWindowPtr());
			engine.PushScene(pauseScene);
		}
		mPrevPausePressed = pressed;

		// Run normal systems/drawing
		BaseScene::Run(engine);
	}

private:
	bool mPrevPausePressed = false;
};

