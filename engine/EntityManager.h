#pragma once

#include <tuple>
#include <unordered_map>
#include <vector>
#include <string>
#include <cassert>
#include "Entity.h"

namespace ECSEngine {

	template <typename T>
	class ComponentStorage {
		public:
			void Add(EntityID id, const T& component) {
				mData[id] = component;
			}

			void Remove(EntityID id) {
				mData.erase(id);
			}

			bool Has(EntityID id) const {
				return mData.find(id) != mData.end();
			}

			T& Get(EntityID id) {
				return mData.at(id);
			}

			const T& Get(EntityID id) const {
				return mData.at(id);
			}

		private:
			std::unordered_map<EntityID, T> mData;
		};

		
	template <typename... Components>
	class EntityManager {

		public:
			EntityID CreateEntity(const std::string& name) {
				EntityID id = mEntities.size();
				mEntities.emplace_back(id, name);
				return id;
			}

	void RemoveEntity(EntityID entity) {}
	
	template <typename T>
	void AddComponent(EntityID entity, T component){}
	
	template <typename T>
	void RemoveComponent(EntityID entity){}
	
	template <typename T>
	bool HasComponent(EntityID entity) const {return false;}
	
	template <typename T>
	T &GetComponent(EntityID entity) { static T tmp; return tmp; }
	
	template <typename T>
	ComponentStorage<T> &GetComponentStorage(){}
	
	
	using tEntity = std::vector<Entity>;
	using const_iterator = tEntity::const_iterator;
	const_iterator cbegin() const { return mEntities.begin(); }
	const_iterator cend() const { return mEntities.end(); }
	const_iterator begin() { return mEntities.begin(); }
	const_iterator end() { return mEntities.end(); }
private:
	template <typename U, typename... Us>
	void RemoveComponentsHelper(EntityID entity);
	
	std::vector<Entity> mEntities;
};

}
