// SpriteManager.h
#pragma once

#include <SFML/Graphics.hpp>
#include <unordered_map>
#include <vector>
#include <string>
#include <limits>
#include <cassert>
#include "MathUtil.h"

namespace ECSEngine {

using SpriteID = size_t;
using TextureID = size_t;

/**
 * @brief Packs textures into a single atlas and provides sprite/UV lookups.
 *
 * @section Lifetime and Validity
 *
 * All textures and sprites are owned by SpriteManager for its lifetime. Sprite
 * IDs returned from RegisterTexture() stay valid until the manager is destroyed.
 * GetTexture() returns a reference valid for the manager's lifetime.
 */
class SpriteManager {
public:
    /**
     * @brief Constructs a sprite atlas with optional padding.
     *
     * @param atlasSize Dimensions of the atlas texture.
     * @param padding   Padding in pixels inserted around packed textures.
     */
    SpriteManager(const sf::Vector2u& atlasSize = {4096, 4096}, uint padding = 0);

    /**
     * @brief Loads a texture from disk and packs it into the atlas.
     *
     * @param filepath   Path to the source image on disk.
     * @param sourceRect Region of the image to pack into the atlas.
     * @return SpriteID  Stable identifier for retrieving the packed sprite.
     */
    [[nodiscard]] SpriteID RegisterTexture(const std::string& filepath, const Rect &sourceRect);

	/**
     * @brief Returns an sf::Sprite configured for the packed texture.
     *
     * @param id SpriteID returned by RegisterTexture().
     * @return sf::Sprite Sprite pointing into the atlas texture.
     */
	sf::Sprite GetSprite(SpriteID id);

    /**
     * @brief Finalizes the atlas and uploads it to the GPU.
     *
     * Must be called after all textures are registered and before rendering.
     */
    void FinalizeAtlas();

    /**
     * @brief Accesses the atlas texture.
     *
     * @return sf::Texture& Reference valid for the manager's lifetime.
     */
    sf::Texture& GetTexture(){return mTexture;}

    /**
     * @brief Retrieves normalized UV coordinates for a packed sprite.
     *
     * @param id SpriteID returned by RegisterTexture().
     * @return Rect UV rectangle in normalized coordinates.
     */
    Rect GetUV(SpriteID id) const;

    /**
     * @brief Returns the pixel size of a packed sprite.
     *
     * @param id SpriteID returned by RegisterTexture().
     * @return sf::Vector2u Width/height in pixels.
     */
    sf::Vector2u GetSize(SpriteID id) const;

private:
    // Packing helpers
    sf::Image padImage(const sf::Image& img, uint pad) const;

    // Internal atlas texture
    sf::Texture mTexture;
    sf::Image mAtlasImage; // used for updates
    sf::Vector2u mAtlasSize;
    uint mPadding;

    // Packing algorithm
    sf::Vector2f mCursor;
    float mLineHeight;
    std::vector<Rect> mRects; // packed positions
    std::unordered_map<SpriteID, sf::Vector2u> mSizes;

    SpriteID mNextID;
};

}
