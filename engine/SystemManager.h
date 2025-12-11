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

/**
 * @brief Manages ownership and execution order of all registered systems.
 *
 * @section Lifetime and Validity
 *
 * Systems are owned by SystemManager for its lifetime and are executed in
 * the order they are added. References returned by GetSystem() remain valid
 * until ClearSystems() is called or the manager is destroyed.
 */
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

	/**
	 * @brief Retrieves a system of the specified concrete type.
	 *
	 * @tparam T Concrete system type to retrieve.
	 * @return T& Reference to the requested system.
	 */
	template <typename T>
	T &GetSystem()
	{
		for (auto &system : mSystems)
		{
			if (auto *casted = dynamic_cast<T *>(system.get()))
				return *casted;
		}
		throw std::runtime_error("Requested system not found in SystemManager.");
	}

	private:
		std::vector<std::unique_ptr<System<Components...>>> mSystems;
	};

} // namespace ECSEngine
