#pragma once

#include <unordered_map>
#include <vector>
#include <SFML/Graphics/Sprite.hpp>
#include <SFML/Graphics/Texture.hpp>
#include <SFML/Graphics/Image.hpp>
#include "MathUtil.h"

namespace ECSEngine
{

	using SpriteID = size_t;

	/**
	 * @brief Manages sprite resources and texture loading for the ECS engine.
	 *
	 * @section Lifetime and Validity
	 *
	 * Textures are stored in an unordered_map and remain valid for the SpriteManager's
	 * lifetime. Sprites are stored in a vector and SpriteIDs remain valid for the
	 * SpriteManager's lifetime. GetSprite() returns a reference that remains valid
	 * until vector reallocation occurs. Storing long-lived references to sprites is
	 * unsafe if RegisterTexture() may be called later.
	 */
	class SpriteManager
	{
	public:
		SpriteManager();
		~SpriteManager() = default;

		/**
		 * @brief Registers a texture and creates a sprite from a rectangle.
		 *
		 * @param texturePath Path to the texture file to load.
		 * @param sourceRect Rectangle defining the portion of the texture to use.
		 * @return SpriteID The ID of the newly created sprite. Returns 0 if loading fails.
		 *
		 * @note The returned SpriteID is valid for the SpriteManager's lifetime.
		 * @warning Calling this function may invalidate previously obtained sprite
		 * references if the vector reallocates.
		 */
		[[nodiscard]] SpriteID RegisterTexture(const std::string &texturePath, const Rect &sourceRect);

		/**
		 * @brief Retrieves a sprite by its ID.
		 *
		 * @param id The SpriteID returned by RegisterTexture().
		 * @return sf::Sprite& Reference to the sprite.
		 *
		 * @note The returned reference is valid until vector reallocation occurs.
		 * @warning Storing long-lived references is unsafe if RegisterTexture() may
		 * be called later. Store SpriteIDs instead.
		 */
		sf::Sprite &GetSprite(SpriteID id);
		sf::Texture &GetTexture() { return mTextureAtlas; }

	private:
		sf::Texture mTextureAtlas;           // the BIG texture containing all sprites
		sf::Image   mAtlasImage;        // <-- keep CPU-side atlas copy

		unsigned int mAtlasWidth;            // current width used
		unsigned int mAtlasHeight;           // current height used

		unsigned int mAtlasCursorX = 0;  // where next sprite will be placed horizontally
		unsigned int mAtlasCursorY = 0;  // where next sprite row starts
		unsigned int mCurrentRowHeight = 0;  // height of the tallest sprite in the current row

		std::vector<sf::Sprite> mSprites;
	};

}
