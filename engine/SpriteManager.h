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

class SpriteManager {
public:
    SpriteManager(const sf::Vector2u& atlasSize = {4096, 4096}, uint padding = 0);

    [[nodiscard]] SpriteID RegisterTexture(const std::string& filepath, const Rect &sourceRect);
	sf::Sprite GetSprite(SpriteID id);
    void FinalizeAtlas();
    sf::Texture& GetTexture(){return mTexture;}

    // UVs for vertex array rendering
    Rect GetUV(SpriteID id) const;
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
