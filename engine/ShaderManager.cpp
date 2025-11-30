#include "ShaderManager.h"
#include <iostream>

namespace ECSEngine
{

	void ShaderManager::RegisterShader(const std::string &shaderPath, const std::string &shaderName)
	{
		sf::Shader shader;
		if (shader.loadFromFile(shaderPath, sf::Shader::Fragment))
		{
			mShaders[shaderName] = std::move(shader);
		}
		else
		{
			std::cerr << "Failed to load shader: " << shaderPath << "\n";
		}
	}

	sf::Shader *ShaderManager::GetShader(const std::string &shaderName)
	{
		auto it = mShaders.find(shaderName);
		if (it != mShaders.end())
		{
			return &it->second;
		}
		return nullptr;
	}

} // namespace ECSEngine
