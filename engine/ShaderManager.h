#pragma once

#include <SFML/Graphics.hpp>
#include <string>
#include <unordered_map>

namespace ECSEngine
{

	/**
	 * @brief Manages shader resources for the ECS engine.
	 */
	class ShaderManager
	{
	public:
		ShaderManager() = default;
		~ShaderManager() = default;

		/**
		 * @brief Registers a shader from a file and stores it under a user-defined name.
		 *
		 * @param shaderPath Path to the shader file to load.
		 * @param shaderName Unique name identifier for the shader.
		 */
		void RegisterShader(const std::string &shaderPath, const std::string &shaderName);

		/**
		 * @brief Gets a pointer to a registered shader by name.
		 *
		 * @param shaderName The name identifier of the shader.
		 * @return sf::Shader* Pointer to the shader, or nullptr if not found.
		 */
		sf::Shader *GetShader(const std::string &shaderName);

	private:
		std::unordered_map<std::string, sf::Shader> mShaders;
	};

} // namespace ECSEngine
