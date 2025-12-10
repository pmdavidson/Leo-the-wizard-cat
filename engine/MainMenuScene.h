#pragma once
#include "Scene.h"
#include <SFML/Graphics.hpp>
#include <SFML/Window/Event.hpp>
#include <memory>

template <typename... Components>
class MainMenuScene : public ECSEngine::Scene<Components...> {
public:
	using BaseScene = ECSEngine::Scene<Components...>;
	using EntityId = size_t;

	explicit MainMenuScene(std::shared_ptr<sf::RenderWindow> window)
		: BaseScene(window), mFontLoaded(false), mStartGame(false) {
		if (mFont.openFromFile("../../assets/fonts/arial.ttf")) {
			mFontLoaded = true;
			mTitle.setFont(mFont);
			mTitle.setString("Leo the Wizard Cat");
			mTitle.setCharacterSize(50);
			mTitle.setFillColor(sf::Color::White);
			mTitle.setPosition(150, 200);

			mPrompt.setFont(mFont);
			mPrompt.setString("Press Enter to Start");
			mPrompt.setCharacterSize(30);
			mPrompt.setFillColor(sf::Color::White);
			mPrompt.setPosition(200, 300);
		}
	}

    void Run(ECSEngine::ECSEngine<Components...>& engine) 
	{
		auto* window = this->GetWindowManager().GetWindow();

        while (auto event = window->pollEvent())
		{
			// Window close
			if (event->template is<sf::Event::Closed>())
                {
                    window->close();
                }

			// Key pressed
			else if (auto key = event->template getIf<sf::Event::KeyPressed>())
			{
				if (key->scancode == sf::Keyboard::Scancode::Enter)
					mStartGame = true;
			}
		}

		window->clear(sf::Color::Black);

		if (mFontLoaded)
		{
			window->draw(mTitle);
			window->draw(mPrompt);
		}

		window->display();  // required
	}

	bool IsComplete() const { return mStartGame; }

private:
	sf::Font mFont;
	sf::Text mTitle;
	sf::Text mPrompt;
	bool mFontLoaded;
	bool mStartGame;
};
