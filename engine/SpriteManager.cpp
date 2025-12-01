#include "SpriteManager.h"
#include <string>
#include <SFML/Graphics.hpp>

namespace ECSEngine
{

    SpriteManager::SpriteManager()
    {
        mAtlasWidth  = 0;
        mAtlasHeight = 0;

        mTextureAtlas.resize(sf::Vector2u(4096,4096), false); // large texture atlas
    }

    SpriteID SpriteManager::RegisterTexture(const std::string& texturePath,
                                        const Rect& sourceRect)
    {
        sf::Image fullImage;
        if (!fullImage.loadFromFile(texturePath))
        {
            std::cerr << "Failed to load image: " << texturePath << "\n";
            return 0;
        }

        const unsigned int spriteW = static_cast<unsigned int>(sourceRect.width);
        const unsigned int spriteH = static_cast<unsigned int>(sourceRect.height);

        // Crop requested region into its own image
        sf::Image subImage(
            sf::Vector2u(spriteW, spriteH),
            sf::Color::Transparent
        );

        subImage.copy(
            fullImage,                                   // source
            sf::Vector2u(0, 0),                          // dest in subImage
            sf::IntRect(                                 // source rect in fullImage
                sf::Vector2i(
                    static_cast<int>(sourceRect.topLeft.x),
                    static_cast<int>(sourceRect.topLeft.y)
                ),
                sf::Vector2i(static_cast<int>(spriteW),
                            static_cast<int>(spriteH))
            ),
            false                                        // no alpha blending
        );

        // Atlas packing: wrap to next row if needed
        if (mAtlasCursorX + spriteW > mAtlasImage.getSize().x)
        {
            mAtlasCursorX = 0;
            mAtlasCursorY += mCurrentRowHeight;
            mCurrentRowHeight = 0;
        }

        mCurrentRowHeight = std::max(mCurrentRowHeight, spriteH);

        if (mAtlasCursorY + spriteH > mAtlasImage.getSize().y)
        {
            std::cerr << "ERROR: Texture atlas overflow (increase atlas size).\n";
            return 0;
        }

        // Copy subImage into atlas at (mAtlasCursorX, mAtlasCursorY)
        mAtlasImage.copy(
            subImage,                                   // source
            sf::Vector2u(mAtlasCursorX, mAtlasCursorY), // dest in atlas
            sf::IntRect(                                // whole subImage
                sf::Vector2i(0, 0),
                sf::Vector2i(static_cast<int>(spriteW),
                            static_cast<int>(spriteH))
            ),
            false                                       // no alpha blending
        );

        // Upload updated atlas to GPU
        mTextureAtlas.update(mAtlasImage);

        // Build sprite pointing into the atlas
        sf::Sprite sprite(mTextureAtlas);
        sprite.setTextureRect(
            sf::IntRect(
                sf::Vector2i(static_cast<int>(mAtlasCursorX),
                            static_cast<int>(mAtlasCursorY)),
                sf::Vector2i(static_cast<int>(spriteW),
                            static_cast<int>(spriteH))
            )
        );

        mSprites.push_back(sprite);
        SpriteID id = mSprites.size() - 1;

        // Advance insertion cursor
        mAtlasCursorX += spriteW;

        return id;
    }

    sf::Sprite &SpriteManager::GetSprite(SpriteID id)
    {
        return mSprites[id];
    }

}
