#pragma once

#include <type_traits>
#include <iostream>
#include <tuple>
#include <array>
#include <vector>
#include <cassert>
#include <limits>

#include "Entity.h"
#include "ComponentStorage.h"

namespace ECSEngine
{

	using EntityID = size_t;

	template <typename T, typename... Ts>
	struct IndexOf;

	template <typename T, typename... Ts>
	struct IndexOf<T, T, Ts...> : std::integral_constant<size_t, 0> {};

	template <typename T, typename U, typename... Ts>
	struct IndexOf<T, U, Ts...> : std::integral_constant<size_t, 1 + IndexOf<T, Ts...>::value> {};

	/**
	 * @brief Manages entities and their components for the ECS engine.
	 *
	 * @section Lifetime and Validity
	 *
	 * EntityIDs are valid until RemoveEntity() is called, after which they may be
	 * reused. GetComponent() returns a reference valid until the component or entity
	 * is removed. Component references are invalidated by RemoveComponent() or
	 * RemoveEntity().
	 *
	 * @tparam Components The component types that can be attached to entities.
	 */
	template <typename... Components>
	class EntityManager
	{

	public:
		EntityManager() = default;

		/**
		 * @brief Creates a new entity and returns its ID.
		 *
		 * @param name Name identifier for the entity.
		 * @return EntityID The ID of the newly created entity.
		 *
		 * @note The returned EntityID is valid until RemoveEntity() is called.
		 * @note Freed entity slots are reused to minimize memory allocation.
		 */
		[[nodiscard]] EntityID CreateEntity(const std::string &name)
		{
			EntityID id;
			if (!mFreeEntityIDs.empty())
			{
				id = mFreeEntityIDs.back();
				mFreeEntityIDs.pop_back();
				mEntities[id] = Entity(id, name);
			}
			else
			{
				id = mEntities.size();
				mEntities.emplace_back(id, name);
			}
			return id;
		}

		/**
		 * @brief Removes an entity and all its components.
		 *
		 * @param entity The EntityID of the entity to remove.
		 *
		 * @note The EntityID may be reused for a new entity after removal.
		 * @warning All component references for this entity become invalid.
		 */
		void RemoveEntity(EntityID entity)
		{
			assert(IsValid(entity));
			RemoveComponentsHelper<Components...>(entity);
			mEntities[entity].setActive(false);
			mFreeEntityIDs.push_back(entity);
		}

		/**
		 * @brief Adds a component to an entity.
		 *
		 * @tparam T The component type. Must be one of the Components template parameters.
		 * @param entity The EntityID of the entity.
		 * @param component The component instance to add.
		 *
		 * @note If the entity already has this component type, it will be replaced.
		 * @warning The component type T must be in the Components template parameter list.
		 */
		template <typename T>
		void AddComponent(EntityID entity, T component)
		{
			constexpr size_t Index = IndexOf<T, Components...>::value;
			static_assert(Index < sizeof...(Components));
			assert(IsValid(entity));

			auto &storage = std::get<Index>(mComponentStorages);
			storage.Store(entity, std::move(component));
		}

		/**
		 * @brief Removes a component from an entity.
		 *
		 * @tparam T The component type. Must be one of the Components template parameters.
		 * @param entity The EntityID of the entity.
		 *
		 * @warning The component type T must be in the Components template parameter list.
		 * @warning Component references of this type for this entity become invalid.
		 */
		template <typename T>
		void RemoveComponent(EntityID entity)
		{
			constexpr size_t Index = IndexOf<T, Components...>::value;
			static_assert(Index < sizeof...(Components));
			assert(IsValid(entity));

			auto &storage = std::get<Index>(mComponentStorages);
			storage.Remove(entity);
		}

		/**
		 * @brief Checks if an entity has a component of the specified type.
		 *
		 * @tparam T The component type. Must be one of the Components template parameters.
		 * @param entity The EntityID of the entity.
		 * @return bool True if the entity has the component, false otherwise.
		 *
		 * @warning The component type T must be in the Components template parameter list.
		 */
		template <typename T>
		bool HasComponent(EntityID entity) const
		{
			constexpr size_t Index = IndexOf<T, Components...>::value;
			static_assert(Index < sizeof...(Components));
			assert(IsValid(entity));

			const auto &storage = std::get<Index>(mComponentStorages);
			return storage.Valid(entity);
		}

		/**
		 * @brief Gets a reference to a component attached to an entity.
		 *
		 * @tparam T The component type. Must be one of the Components template parameters.
		 * @param entity The EntityID of the entity.
		 * @return T& Reference to the component.
		 *
		 * @note The returned reference is valid until RemoveComponent() or RemoveEntity()
		 * is called for this component/entity.
		 * @warning The component type T must be in the Components template parameter list.
		 * @warning The entity must have the component. Use HasComponent() to check first.
		 */
		template <typename T>
		T &GetComponent(EntityID entity)
		{
			constexpr size_t Index = IndexOf<T, Components...>::value;
			static_assert(Index < sizeof...(Components));
			assert(IsValid(entity));

			auto &storage = std::get<Index>(mComponentStorages);
			return storage.Get(entity);
		}

		/**
		 * @brief Gets the component storage for a specific component type.
		 *
		 * @tparam T The component type. Must be one of the Components template parameters.
		 * @return ComponentStorage<T>& Reference to the component storage.
		 *
		 * @warning The component type T must be in the Components template parameter list.
		 */
		template <typename T>
		ComponentStorage<T> &GetComponentStorage()
		{
			constexpr size_t Index = IndexOf<T, Components...>::value;
			static_assert(Index < sizeof...(Components));
			return std::get<Index>(mComponentStorages);
		}

		using tEntity = std::vector<Entity>;
		using const_iterator = typename tEntity::const_iterator;

		/**
		 * @brief Returns a const iterator to the beginning of the entities.
		 */
		const_iterator cbegin() const { return mEntities.cbegin(); }
		
		/**
		 * @brief Returns a const iterator to the end of the entities.
		 */
		const_iterator cend() const { return mEntities.cend(); }
		
		/**
		 * @brief Returns an iterator to the beginning of the entities.
		 */
		const_iterator begin() { return mEntities.begin(); }
		
		/**
		 * @brief Returns an iterator to the end of the entities.
		 */
		const_iterator end() { return mEntities.end(); }

	private:
		std::vector<Entity> mEntities;
		std::vector<EntityID> mFreeEntityIDs;
		std::tuple<ComponentStorage<Components>...> mComponentStorages;

		bool IsValid(EntityID id) const
		{
			return id < mEntities.size() && mEntities[id].isActive();
		}

		template <typename U, typename... Us>
		void RemoveComponentsHelper(EntityID entity) {
			if (HasComponent<U>(entity)) {
				RemoveComponent<U>(entity);
			}

			if constexpr (sizeof...(Us) > 0) {
				RemoveComponentsHelper<Us...>(entity);
			}
		}

	};
}
