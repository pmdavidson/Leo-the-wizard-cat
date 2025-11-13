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

	template <typename... Components>
	class EntityManager
	{

	public:
		EntityManager() = default;

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

		void RemoveEntity(EntityID entity)
		{
			assert(IsValid(entity));
			RemoveComponentsHelper<Components...>(entity);
			mEntities[entity].setActive(false);
			mFreeEntityIDs.push_back(entity);
		}

		template <typename T>
		void AddComponent(EntityID entity, T component)
		{
			constexpr size_t Index = IndexOf<T, Components...>::value;
			static_assert(Index < sizeof...(Components));
			assert(IsValid(entity));

			auto &storage = std::get<Index>(mComponentStorages);
			storage.Store(entity, std::move(component));
		}

		template <typename T>
		void RemoveComponent(EntityID entity)
		{
			constexpr size_t Index = IndexOf<T, Components...>::value;
			static_assert(Index < sizeof...(Components));
			assert(IsValid(entity));

			auto &storage = std::get<Index>(mComponentStorages);
			storage.Remove(entity);
		}

		template <typename T>
		bool HasComponent(EntityID entity) const
		{
			constexpr size_t Index = IndexOf<T, Components...>::value;
			static_assert(Index < sizeof...(Components));
			assert(IsValid(entity));

			const auto &storage = std::get<Index>(mComponentStorages);
			return storage.Valid(entity);
		}

		template <typename T>
		T &GetComponent(EntityID entity)
		{
			constexpr size_t Index = IndexOf<T, Components...>::value;
			static_assert(Index < sizeof...(Components));
			assert(IsValid(entity));

			auto &storage = std::get<Index>(mComponentStorages);
			return storage.Get(entity);
		}

		template <typename T>
		ComponentStorage<T> &GetComponentStorage()
		{
			constexpr size_t Index = IndexOf<T, Components...>::value;
			static_assert(Index < sizeof...(Components));
			return std::get<Index>(mComponentStorages);
		}

		using tEntity = std::vector<Entity>;
		using const_iterator = typename tEntity::const_iterator;

		const_iterator cbegin() const { return mEntities.cbegin(); }
		const_iterator cend() const { return mEntities.cend(); }
		const_iterator begin() { return mEntities.begin(); }
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
