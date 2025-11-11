#pragma once

#include <unordered_map>
#include <vector>
#include <SFML/Graphics/Sprite.hpp>
#include <SFML/Graphics/Texture.hpp>
#include "MathUtil.h"

namespace ECSEngine
{

	using SpriteID = size_t;

	class SpriteManager
	{
	public:
		SpriteManager() = default;
		~SpriteManager() = default;
		[[nodiscard]] SpriteID RegisterTexture(const std::string &texturePath, const Rect &sourceRect);
		sf::Sprite &GetSprite(SpriteID id);

	private:
		std::unordered_map<std::string, sf::Texture> mTextures;
		std::vector<sf::Sprite> mSprites;
	};

}
