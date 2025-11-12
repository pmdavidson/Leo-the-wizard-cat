#pragma once

#include <unordered_map>
#include <vector>
#include <SFML/Graphics/Sprite.hpp>
#include <SFML/Graphics/Texture.hpp>
#include "MathUtil.h"

namespace ECSEngine
{

	using SpriteID = size_t;

	/**
	 * @brief Manages sprite resources and texture loading for the ECS engine.
	 * 
	 * The SpriteManager handles texture loading, caching, and sprite creation.
	 * Textures are cached by file path, so loading the same texture multiple times
	 * will reuse the cached texture. Sprites are stored in a vector and accessed
	 * by their SpriteID.
	 * 
	 * @section resource_lifetime Resource Lifetime and Validity
	 * 
	 * Textures are stored in an unordered_map and remain valid for the entire
	 * lifetime of the SpriteManager instance. Once a texture is loaded, it persists
	 * until the SpriteManager is destroyed. Multiple sprites can reference the same
	 * texture (textures are shared).
	 * 
	 * Sprites are stored in a vector and remain valid for the entire lifetime of the
	 * SpriteManager instance. SpriteIDs returned by RegisterTexture() are valid
	 * indices into this vector and remain valid until the SpriteManager is destroyed.
	 * 
	 * GetSprite() returns a reference to an sf::Sprite stored in the internal vector.
	 * This reference is valid as long as the SpriteManager instance exists and no
	 * operations cause vector reallocation (e.g., adding sprites that exceed the
	 * vector's capacity). In practice, sprites are typically registered during
	 * initialization, so reallocation is unlikely during gameplay. However, if
	 * RegisterTexture() is called after storing a reference, the reference may become
	 * invalid if the vector reallocates.
	 * 
	 * @warning Storing long-lived references to sprites is unsafe if RegisterTexture()
	 * as vector reallocation will invalidate the
	 *          reference. Prefer storing SpriteIDs and calling GetSprite() when needed.
	 */
	class SpriteManager
	{
	public:
		SpriteManager() = default;
		~SpriteManager() = default;

		/**
		 * @brief Registers a texture and creates a sprite from a source rectangle.
		 * 
		 * If the texture at the given path has already been loaded, it will be
		 * reused. A new sprite is created from the texture using the specified
		 * source rectangle, and the sprite is added to the internal sprite vector.
		 * 
		 * @param texturePath Path to the texture file to load.
		 * @param sourceRect Rectangle defining the portion of the texture to use
		 *                   for the sprite (in texture coordinates).
		 * @return SpriteID The ID of the newly created sprite. This ID can be used
		 *                  with GetSprite() to retrieve the sprite. Returns 0 if
		 *                  texture loading fails.
		 * 
		 * @note The returned SpriteID is valid for the lifetime of the SpriteManager.
		 * @note The sprite itself is stored in the internal vector and persists until
		 *       the SpriteManager is destroyed.
		 * @note If texture loading fails, the function prints an error message and
		 *       returns 0. The caller should check for this case.
		 * @warning Calling this function may invalidate previously obtained sprite
		 *          references if the vector reallocates. Store SpriteIDs instead
		 *          of references if RegisterTexture() may be called later.
		 */
		[[nodiscard]] SpriteID RegisterTexture(const std::string &texturePath, const Rect &sourceRect);

		/**
		 * @brief Retrieves a sprite by its ID.
		 * 
		 * @param id The SpriteID returned by RegisterTexture(). Must be a valid
		 *            ID (0 is returned on error, but accessing it is undefined).
		 * @return sf::Sprite& Reference to the sprite stored in the internal vector.
		 * 
		 * @note The returned reference is valid as long as the SpriteManager instance
		 *       exists and no subsequent call to RegisterTexture() causes vector
		 *       reallocation. In typical usage (sprites registered at initialization),
		 *       the reference remains valid for the lifetime of the SpriteManager.
		 * @note The reference can be used to modify sprite properties (position,
		 *       rotation, scale, color, etc.). Modifications persist for the sprite's
		 *       lifetime.
		 * @warning The caller must ensure that the provided ID is valid. Accessing
		 *          an invalid ID results in undefined behavior (vector bounds violation).
		 * @warning Storing long-lived references to sprites is unsafe if
		 *          RegisterTexture() may be called later, as vector reallocation
		 *          will invalidate the reference. Prefer storing SpriteIDs and
		 *          calling GetSprite() when needed.
		 */
		sf::Sprite &GetSprite(SpriteID id);

	private:
		std::unordered_map<std::string, sf::Texture> mTextures;
		std::vector<sf::Sprite> mSprites;
	};

}
