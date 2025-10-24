#pragma once

#include <type_traits>
#include <iostream>
#include "Entity.h"
#include "ComponentStorage.h"
#include <cassert>


namespace ECSEngine
{

template <typename... Components>
class EntityManager {
public:
	[[nodiscard]] EntityID CreateEntity(const std::string &name) { return 0; }

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
