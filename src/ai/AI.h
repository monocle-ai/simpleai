#pragma once

#include <unordered_map>

#include "aggro/AggroMgr.h"
#include "tree/TreeNode.h"
#include "tree/loaders/ITreeLoader.h"
#include "group/GroupMgr.h"
#include "common/Thread.h"

namespace ai {

class ICharacter;
class Zone;

typedef std::vector<CharacterId> FilteredEntities;

#define NOTHING_SELECTED (-1)

/**
 * @brief This is the type the library works with. It interacts with it's real world entity by
 * the @c ICharacter interface.
 *
 * Each ai entity has a @c AggroMgr assigned that is updated with each tick. The character also
 * holds a reference to the @c IPathfinder that should be used to move the attached @c ICharacter
 * through the world.
 *
 * A behaviour can be replaced at runtime with @c AI::setBehaviour
 *
 * You can set single @c AI instances to no longer update their state by calling @c AI::setPause
 */
class AI {
	friend class TreeNode;
	friend class IFilter;
	friend class Filter;
	friend class Server;
protected:
	/**
	 * This map is only filled if we are in debugging mode for this entity
	 */
	typedef std::unordered_map<int, TreeNodeStatus> NodeStates;
	NodeStates _lastStatus;
	/**
	 * This map is only filled if we are in debugging mode for this entity
	 */
	typedef std::unordered_map<int, long> LastExecMap;
	LastExecMap _lastExecMillis;

	mutable FilteredEntities _filteredEntities;

	/**
	 * Often @c Selector states must be stored to continue in the next step at a particular
	 * position in the behaviour tree. This map is doing exactly this.
	 */
	typedef std::unordered_map<int, int> SelectorStates;
	SelectorStates _selectorStates;

	TreeNodePtr _behaviour;
	AggroMgr _aggroList;

	ICharacter& _character;

	bool _pause;
	bool _debuggingActive;

	long _time;

	Zone* _zone;

	ATOMIC(bool) _reset;
public:
	/**
	 * @param character The binding to your game entity
	 * @param behaviour The behaviour tree node that is applied to this ai entity
	 */
	AI(ICharacter& character, const TreeNodePtr& behaviour);
	virtual ~AI();

	/**
	 * @brief Update the behaviour and the aggro values if the entity is not on hold.
	 * @param[in] dt The current milliseconds to update the aggro entries and
	 * time based tasks or conditions.
	 */
	virtual void update(long dt, bool debuggingActive);

	/**
	 * @brief Set the new @c Zone this entity is in
	 */
	void setZone(Zone* zone);
	/**
	 * Returns the zone this entity is in. Make sure to call @c hasZone before.
	 */
	Zone& getZone() const;
	/**
	 * @brief Returns @c true if the entity is already in a zone. This must not be managed manually,
	 * the @c Zone is doing that already.
	 */
	bool hasZone() const;

	/**
	 * @brief don't update the entity as long as it is paused
	 * @sa isPause()
	 */
	void setPause(bool pause);

	/**
	 * @brief don't update the entity as long as it is paused
	 * @sa setPause()
	 */
	bool isPause() const;

	bool isDebuggingActive() const;

	Vector3f getGroupPosition(GroupId id) const;

	/**
	 * @brief Get the current behaviour for this ai
	 */
	TreeNodePtr getBehaviour() const;
	/**
	 * @brief Set a new behaviour and return the old one
	 */
	TreeNodePtr setBehaviour(const TreeNodePtr& newBehaviour);
	/**
	 * @return The real world entity reference
	 */
	ICharacter& getCharacter() const;
	/**
	 * @return the @c AggroMgr for this @c AI instance. Each @c AI instance has its own @c AggroMgr instance.
	 */
	AggroMgr& getAggroMgr();
	/**
	 * @return the @c Zone's @c GroupMgr instance to modify
	 */
	const AggroMgr& getAggroMgr() const;

	/**
	 * @brief @c FilteredEntities is holding a list of @c CharacterIds that were selected by the @c Select condition.
	 * @sa @c IFilter interface.
	 * @sa @c Filter condition that executes assigned @c IFilter implementations.
	 * @return A reference to the internal data structure. This should only be used from within @c TreeNode implementations
	 * to access those entities that were filtered by the @c Filter condition.
	 *
	 * @note If you call this from outside of the behaviour tree tick, you will run into race conditions.
	 */
	const FilteredEntities& getFilteredEntities() const;
};

inline TreeNodePtr AI::getBehaviour() const {
	return _behaviour;
}

inline TreeNodePtr AI::setBehaviour(const TreeNodePtr& newBehaviour) {
	TreeNodePtr current = _behaviour;
	_behaviour = newBehaviour;
	_reset = true;
	return current;
}

inline void AI::setPause(bool pause) {
	_pause = pause;
}

inline bool AI::isPause() const {
	return _pause;
}

inline ICharacter& AI::getCharacter() const {
	return _character;
}

inline AggroMgr& AI::getAggroMgr() {
	return _aggroList;
}

inline const AggroMgr& AI::getAggroMgr() const {
	return _aggroList;
}

inline const FilteredEntities& AI::getFilteredEntities() const {
	return _filteredEntities;
}

inline bool AI::isDebuggingActive() const {
	return _debuggingActive;
}

inline void AI::setZone(Zone* zone) {
	_zone = zone;
}

inline Zone& AI::getZone() const {
	return *_zone;
}

inline bool AI::hasZone() const {
	return _zone != nullptr;
}

}
