#pragma once
#include "Scene.h"
#include <SFML/Graphics.hpp>
#include <SFML/Window/Event.hpp>
#include <filesystem>
#include <iostream>
#include <memory>

extern std::string gResourcePath;

template <typename... Components>
class MainMenuScene : public ECSEngine::Scene<Components...> {
public:
	using BaseScene = ECSEngine::Scene<Components...>;
	using EntityId = size_t;

	explicit MainMenuScene(std::shared_ptr<sf::RenderWindow> window)
		: BaseScene(window), mTitle(mFont), mPrompt(mFont), mFontLoaded(false), mStartGame(false), mLogged(false) {
		mFontPath = gResourcePath + "fonts/ARIAL.TTF";
		if (mFont.openFromFile(mFontPath)) {
			mFontLoaded = true;
			mTitle.setFont(mFont);
			mTitle.setString("Leo the Wizard Cat");
			mTitle.setCharacterSize(50);
			mTitle.setFillColor(sf::Color::White);
			mTitle.setPosition(sf::Vector2f(150.f, 200.f));

			mPrompt.setFont(mFont);
			mPrompt.setString("Press Enter to Start");
			mPrompt.setCharacterSize(30);
			mPrompt.setFillColor(sf::Color::White);
			mPrompt.setPosition(sf::Vector2f(200.f, 300.f));
		} 
	}

    void Run(ECSEngine::ECSEngine<Components...>& engine) 
	{
		(void)engine;
		auto* window = this->GetWindowManager().GetWindow();

		if (!mLogged)
		{
			mLogged = true;
			if (mFontLoaded)
			{
				std::cout << "MainMenuScene: font loaded from " 
						  << std::filesystem::absolute(mFontPath) << "\n";
			}
			else
			{
				std::cerr << "MainMenuScene: failed to load font at "
						  << std::filesystem::absolute(mFontPath)
						  << " (cwd=" << std::filesystem::current_path() << ")\n";
			}
		}

		// Ensure default view in case gameplay scene changed it
		window->setView(window->getDefaultView());

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

		// Draw a fallback background rectangle so we know we're rendering
		sf::RectangleShape backdrop(sf::Vector2f(static_cast<float>(window->getSize().x),
												 static_cast<float>(window->getSize().y)));
		backdrop.setFillColor(sf::Color(20, 20, 60));
		window->draw(backdrop);

		if (mFontLoaded)
		{
			window->draw(mTitle);
			window->draw(mPrompt);
		}
	}


	bool IsComplete() const { return mStartGame; }

private:
	sf::Font mFont;
	sf::Text mTitle;
	sf::Text mPrompt;
	std::string mFontPath;
	bool mFontLoaded;
	bool mStartGame;
	bool mLogged;
};
