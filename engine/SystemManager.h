#pragma once

#include <memory>
#include <vector>
#include "ShaderManager.h"

namespace ECSEngine
{

	template <typename... Components>
	class Scene;

	template <typename... Components>
	class System
	{
	public:
		virtual ~System() = default;

		/**
		 * @brief Runs the systems.
		 *
		 * @param scene Reference to the scene containing all managers and entities.
		 * @return true on success, false on failure.
		 */
		virtual bool Run(Scene<Components...> &scene) = 0;
	};

	template <typename... Components>
	class SystemManager
	{
	public:
		SystemManager() = default;

		/**
		 * @brief Adds a system to the manager.
		 *
		 * @param system Unique pointer to the system to add.
		 */
		void AddSystem(std::unique_ptr<System<Components...>> &&system)
		{
			mSystems.push_back(std::move(system));
		}

		/**
		 * @brief Removes all systems from the manager.
		 */
		void ClearSystems()
		{
			mSystems.clear();
		}

		/**
		 * @brief Runs all systems in the order they were added.
		 *
		 * @param scene Reference to the scene containing all managers and entities.
		 * @return true if all systems succeeded, false if any system failed.
		 */
		bool Run(Scene<Components...> &scene)
		{
			for (auto &system : mSystems)
			{
				if (!system->Run(scene))
					return false; // abort if any system fails
			}
			return true;
		}

	private:
		std::vector<std::unique_ptr<System<Components...>>> mSystems;
	};

} // namespace ECSEngine
