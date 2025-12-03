#include "SpriteManager.h"
#include <iostream>
#include <algorithm>

namespace ECSEngine {

    SpriteManager::SpriteManager(const sf::Vector2u& atlasSize, uint padding)
        : mAtlasSize(atlasSize), mPadding(padding), mCursor(0.f, 0.f), mLineHeight(0.f), mNextID(0)
    {
        mTexture.resize(atlasSize, false);
        mTexture.setSmooth(false); // Optional
    }

    sf::Image SpriteManager::padImage(const sf::Image& img, uint pad) const {
        const sf::Vector2u size = img.getSize();
        sf::Image out(sf::Vector2u(size.x + 2 * pad, size.y + 2 * pad), sf::Color::Transparent);

        for (unsigned int y = 0; y < out.getSize().y; ++y) {
            for (unsigned int x = 0; x < out.getSize().x; ++x) {
                int srcX = std::min(std::max(int(x) - int(pad), 0), int(size.x) - 1);
                int srcY = std::min(std::max(int(y) - int(pad), 0), int(size.y) - 1);
                out.setPixel(sf::Vector2u(x, y), img.getPixel(sf::Vector2u(srcX, srcY)));
            }
        }

        return out;
    }

    SpriteID SpriteManager::RegisterTexture(const std::string& filepath) {
        sf::Image original;
        if (!original.loadFromFile(filepath)) {
            std::cerr << "Failed to load image: " << filepath << "\n";
            return 0;
        }
        // mTexture = sf::Texture(filepath, false);

        sf::Image padded = padImage(original, mPadding);
        sf::Vector2u paddedSize = padded.getSize();

        // New row if needed
        if (mCursor.x + paddedSize.x > mAtlasSize.x) {
            mCursor.x = 0.f;
            mCursor.y += mLineHeight;
            mLineHeight = 0.f;
        }

        if (mCursor.y + paddedSize.y > mAtlasSize.y) {
            std::cerr << "Texture atlas is full! Cannot pack: " << filepath << "\n";
            return 0;
        }
        
        // Record bounds
        Rect rect(
            mCursor.x + mPadding,
            mCursor.y + mPadding,
            static_cast<float>(padded.getSize().x),
            static_cast<float>(padded.getSize().y)
        );
        mRects.push_back(rect);

        // Blit into atlas
        if (!mAtlasImage.copy(padded, sf::Vector2u(mCursor.x, mCursor.y))){
            std::cerr << "Failed to upload image: " << filepath << "to Atlas in copy section\n";
            return 0;
        }

        // mTexture.update(
        //     padded.getPixelsPtr(),             // const Uint8* pixels
        //     sf::Vector2u(padded.getSize().x, padded.getSize().y), // width and height
        //     sf::Vector2u(static_cast<unsigned int>(rect.topLeft.x),  // x offset
        //     static_cast<unsigned int>(rect.topLeft.y))    // y offset
        // );
        
        mSizes[mNextID] = original.getSize();

        // Update cursor
        mCursor.x += paddedSize.x;
        mLineHeight = std::max(mLineHeight, static_cast<float>(paddedSize.y));

        return mNextID++;
    }

    sf::Sprite& SpriteManager::GetSprite(SpriteID id) {
        sf::Sprite sprite(mTexture);
        Rect bounds(mRects.at(id));

        sprite.setTextureRect(
        sf::IntRect(
                { static_cast<int>(bounds.topLeft.x), static_cast<int>(bounds.topLeft.y) },
                { static_cast<int>(bounds.width),   static_cast<int>(bounds.height) }
            )
        );

        return sprite;
    }

    const sf::Texture& SpriteManager::GetTexture() const {
        const_cast<sf::Texture&>(mTexture).loadFromImage(mAtlasImage);
        return mTexture;
    }

    Rect SpriteManager::GetUV(SpriteID id) const {
        assert(mSizes.find(id) != mSizes.end());
        sf::Vector2f atlasSize(mAtlasSize);
        Rect bounds = mRects.at(id); // mRects stores padded position + unpadded size

        Point2D TopLeft(bounds.topLeft.x + mPadding, bounds.topLeft.y + mPadding);
        sf::Vector2f size = static_cast<sf::Vector2f>(mSizes.at(id)); // unpadded size

        sf::Vector2f uv0(TopLeft.x / atlasSize.x, TopLeft.y / atlasSize.y);
        sf::Vector2f uv1((TopLeft.x + size.x) / atlasSize.x, (TopLeft.y + size.y) / atlasSize.y);

        return Rect(
            uv0.x,
            1.0f - uv1.y,                // Y-flip for OpenGL style
            uv1.x - uv0.x,
            uv1.y - uv0.y
        );
    }

    sf::Vector2u SpriteManager::GetSize(SpriteID id) const {
        return mSizes.at(id);
    }

}
