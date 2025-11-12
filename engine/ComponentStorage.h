#pragma once

#include <vector>
#include <unordered_map>
#include <utility>
#include <cassert>

namespace ECSEngine {

using EntityID = size_t;

template <typename T>
class ComponentStorage {
public:
    ComponentStorage() = default;

    // Store a component for an entity, reusing free slots when possible.
    template <typename U>
    size_t Store(EntityID entity, U&& value) {
        // If this entity already has a component, replace it.
        if (mEntityToIndex.contains(entity)) {
            size_t idx = mEntityToIndex[entity];
            mStorage[idx] = std::forward<U>(value);
            mValid[idx] = true;
            return idx;
        }

        size_t idx;
        if (!mFreeList.empty()) {
            idx = mFreeList.back();
            mFreeList.pop_back();
            mStorage[idx] = std::forward<U>(value);
            mValid[idx] = true;
        } else {
            idx = mStorage.size();
            mStorage.emplace_back(std::forward<U>(value));
            mValid.emplace_back(true);
        }

        mEntityToIndex[entity] = idx;
        return idx;
    }

    // Remove a component belonging to an entity.
    void Remove(EntityID entity) {
        if (!mEntityToIndex.contains(entity)) return;
        size_t idx = mEntityToIndex[entity];
        mValid[idx] = false;
        mFreeList.push_back(idx);
        mEntityToIndex.erase(entity);
        mStorage[idx] = T{};
    }

    // Check if an entity has a component.
    bool Valid(EntityID entity) const {
        return mEntityToIndex.contains(entity) && mValid[mEntityToIndex.at(entity)];
    }

    // Get component by entity.
    T& Get(EntityID entity) {
        assert(Valid(entity));
        return mStorage[mEntityToIndex.at(entity)];
    }

    const T& Get(EntityID entity) const {
        assert(Valid(entity));
        return mStorage[mEntityToIndex.at(entity)];
    }

private:
    std::vector<T> mStorage;
	std::vector<bool> mValid;	   // Quickly track if a component is valid
	std::vector<size_t> mFreeList; // Avoid scanning for free slots
    std::unordered_map<EntityID, size_t> mEntityToIndex;
};

} // namespace ECSEngine
