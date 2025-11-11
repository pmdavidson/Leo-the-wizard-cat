#include "SpriteManager.h"
#include <string>

namespace ECSEngine
{

    SpriteID SpriteManager::RegisterTexture(const std::string &texturePath, const Rect &sourceRect)
    {
        // Check if texture is already loaded
        auto it = mTextures.find(texturePath);
        if (it == mTextures.end())
        {
            // Load the texture if not already loaded
            sf::Texture texture;
            if (!texture.loadFromFile(texturePath))
            {
                // Handle error
                std::cout << "Failed to load texture";
                return 0;
            }
            mTextures.emplace(texturePath, std::move(texture));
        }

        // Create a sprite from the texture using the given rectangle
        sf::Sprite sprite(mTextures[texturePath]);
        sprite.setTextureRect(sf::IntRect(
            sf::Vector2i(static_cast<int>(sourceRect.topLeft.x), static_cast<int>(sourceRect.topLeft.y)),
            sf::Vector2i(sourceRect.width, sourceRect.height)));

        // Add sprite to vector and return its index
        mSprites.push_back(sprite);
        return mSprites.size() - 1;
    }

    sf::Sprite &SpriteManager::GetSprite(SpriteID id)
    {
        return mSprites[id];
    }

}
