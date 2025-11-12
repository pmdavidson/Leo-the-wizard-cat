#pragma once

#include <tuple>
#include <unordered_map>
#include <vector>
#include <string>
#include <cassert>
#include "Entity.h"

namespace ECSEngine
{

	template <typename T>
	class ComponentStorage
	{
	public:
		void Add(EntityID id, const T &component)
		{
			mData[id] = component;
		}

		void Remove(EntityID id)
		{
			mData.erase(id);
		}

		bool Has(EntityID id) const
		{
			return mData.find(id) != mData.end();
		}

		T &Get(EntityID id)
		{
			return mData.at(id);
		}

		const T &Get(EntityID id) const
		{
			return mData.at(id);
		}

	private:
		std::unordered_map<EntityID, T> mData;
	};

	template <typename... Components>
	class EntityManager
	{

	public:
		EntityID CreateEntity(const std::string &name)
		{
			EntityID id = mEntities.size();
			mEntities.emplace_back(id, name);
			return id;
		}

		void RemoveEntity(EntityID id)
		{
			if (IsValid(id))
			{
				mEntities[id].setActive(false);
				RemoveComponentsHelper<Components...>(id);
			}
		}

		bool IsValid(EntityID id) const
		{
			return id < mEntities.size() && mEntities[id].isActive();
		}

		template <typename T>
		void AddComponent(EntityID id, const T &component)
		{
			assert(IsValid(id));
			GetComponentStorage<T>().Add(id, component);
		}

		template <typename T>
		void RemoveComponent(EntityID id)
		{
			assert(IsValid(id));
			GetComponentStorage<T>().Remove(id);
		}

		template <typename T>
		bool HasComponent(EntityID id) const
		{
			assert(id < mEntities.size());
			return std::get<ComponentStorage<T>>(mComponentStorages).Has(id);
		}

		template <typename T>
		T &GetComponent(EntityID id)
		{
			assert(IsValid(id));
			return GetComponentStorage<T>().Get(id);
		}

		template <typename T>
		const T &GetComponent(EntityID id) const
		{
			assert(IsValid(id));
			return std::get<ComponentStorage<T>>(mComponentStorages).Get(id);
		}

		template <typename T>
		ComponentStorage<T> &GetComponentStorage()
		{
			return std::get<ComponentStorage<T>>(mComponentStorages);
		}

		using tEntity = std::vector<Entity>;
		using const_iterator = tEntity::const_iterator;

		const_iterator cbegin() const { return mEntities.cbegin(); }
		const_iterator cend() const { return mEntities.cend(); }
		const_iterator begin() { return mEntities.begin(); }
		const_iterator end() { return mEntities.end(); }

	private:
		std::vector<Entity> mEntities;
		std::tuple<ComponentStorage<Components>...> mComponentStorages;

		void RemoveComponentsHelper(EntityID) {}

		template <typename U, typename... Us>
		void RemoveComponentsHelper(EntityID entity)
		{
			RemoveComponent<U>(entity);
			RemoveComponentsHelper<Us...>(entity);
		}
	};
}
