#pragma once

#include <vector>
#include <string>
#include <unordered_map>

#include <SFML/Graphics/VertexArray.hpp>
#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/Graphics/Shader.hpp>

#include "SystemManager.h"
#include "Scene.h"
#include "SpriteComponent.h"
#include "LocationComponent.h"
#include "SpriteManager.h"
#include "WindowManager.h"
#include "MathUtil.h"

namespace ECSEngine
{
class ShaderManager
{
public:
    void RegisterShader(const std::string &shaderPath, const std::string &shaderName)
    {
        if (mShaders.find(shaderName) != mShaders.end())
            return;

        auto shader = std::make_unique<sf::Shader>();

        if (!shader->loadFromFile(shaderPath, sf::Shader::Type::Fragment))
        {
            std::cerr << "Failed to load shader: " << shaderPath << "\n";
            return;
        }

        mShaders.emplace(shaderName, std::move(shader));
    }

    sf::Shader *GetShader(const std::string &shaderName)
    {
        auto it = mShaders.find(shaderName);
        return (it != mShaders.end()) ? it->second.get() : nullptr;
    }

private:
    std::unordered_map<std::string, std::unique_ptr<sf::Shader>> mShaders;
};

} // namespace ECSEngine
