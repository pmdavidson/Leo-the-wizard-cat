#pragma once
#include "Scene.h"
#include <SFML/Graphics.hpp>
#include <SFML/Window/Event.hpp>
#include <memory>

extern std::string gResourcePath;

template <typename... Components>
class PauseScene : public ECSEngine::Scene<Components...> {
public:
	using BaseScene = ECSEngine::Scene<Components...>;

	explicit PauseScene(std::shared_ptr<sf::RenderWindow> window)
		: BaseScene(window), mTitle(mFont), mPrompt(mFont), mFontLoaded(false), mResume(false), mLogged(false) {
		mFontPath = gResourcePath + "fonts/ARIAL.TTF";
		if (mFont.openFromFile(mFontPath)) {
			mFontLoaded = true;
			SetupText(mTitle, "Paused", 48, sf::Vector2f(120.f, 180.f));
			SetupText(mPrompt, "Press Enter or Esc to resume", 28, sf::Vector2f(120.f, 260.f));
		}
	}

	void Run(ECSEngine::ECSEngine<Components...> &engine)
	{
		(void)engine;
		auto* window = this->GetWindowManager().GetWindow();

		if (!mLogged) {
			mLogged = true;
			if (mFontLoaded) {
				std::cout << "PauseScene: font loaded from " << mFontPath << "\n";
			} else {
				std::cerr << "PauseScene: failed to load font at " << mFontPath << "\n";
			}
		}

		window->setView(window->getDefaultView());

		while (auto event = window->pollEvent())
		{
			if (event->template is<sf::Event::Closed>()) {
				window->close();
			}
			else if (auto key = event->template getIf<sf::Event::KeyPressed>()) {
				if (key->scancode == sf::Keyboard::Scancode::Enter ||
					key->scancode == sf::Keyboard::Scancode::Escape) {
					mResume = true;
				}
			}
		}

		window->clear(sf::Color(0, 0, 0, 255));

		// dark overlay
		sf::RectangleShape overlay(sf::Vector2f(static_cast<float>(window->getSize().x),
												static_cast<float>(window->getSize().y)));
		overlay.setFillColor(sf::Color(0, 0, 0, 180));
		window->draw(overlay);

		if (mFontLoaded) {
			window->draw(mTitle);
			window->draw(mPrompt);
		}

		if (mResume) {
			this->SetComplete();
		}
	}

	bool IsComplete() const { return mResume; }

private:
	void SetupText(sf::Text &text, const std::string &str, unsigned int size, sf::Vector2f pos)
	{
		text.setFont(mFont);
		text.setString(str);
		text.setCharacterSize(size);
		text.setFillColor(sf::Color::White);
		text.setPosition(pos);
	}

	sf::Font mFont;
	sf::Text mTitle;
	sf::Text mPrompt;
	std::string mFontPath;
	bool mFontLoaded;
	bool mResume;
	bool mLogged;
};

