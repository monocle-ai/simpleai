/**
 * @file
 * @ingroup LUA
 */
#pragma once

#include "AIRegistry.h"
#include "LUAFunctions.h"
#include "tree/LUATreeNode.h"
#include "conditions/LUACondition.h"
#include "filter/LUAFilter.h"
#include "movement/LUASteering.h"

namespace ai {

/**
 * @brief Allows you to register lua @ai{TreeNode}s, @ai{Conditions}, @ai{Filter}s and @ai{ISteering}s.
 *
 * @see @ai{LUATreeNode}
 *
 * @par TreeNode
 * @code
 * local luanode = REGISTRY.createNode("SomeName")
 * function luanode:execute(ai, deltaMillis)
 *   print("Node execute called with parameters: ai=["..tostring(ai).."], deltaMillis=["..tostring(deltaMillis).."]")
 *   return FINISHED
 * end
 * @encode
 * The @ai{TreeNodeStatus} states are put into the global space. They are: @c UNKNOWN, @c CANNOTEXECUTE,
 * @c RUNNING, @c FINISHED, @c FAILED and @c EXCEPTION
 *
 * Use @c SomeName later on in your behaviour trees to use this @ai{ITreeNode}
 *
 * @par Conditions
 * @code
 * local luacondition = REGISTRY.createCondition("SomeName")
 * function luacondition:evaluate(ai)
 *   --print("Condition evaluate called with parameter: ai=["..tostring(ai).."]")
 *   return true
 * end
 * @encode
 *
 * Use @c SomeName later on in your behaviour trees to use this @ai{ICondition}
 *
 * @par IFilter
 * @code
 * local luafilter = REGISTRY.createFilter("SomeName")
 * function luafilter:filter(ai)
 * end
 * @endcode
 *
 * Use @c SomeName later on in your behaviour trees to use this @ai{ICondition}
 *
 * @par ISteering
 * @code
 * local luasteering = REGISTRY.createSteering("SomeName")
 * function luasteering:execute(ai, speed)
 *   -- return MoveVector
 *   return 0.0, 1.0, 0.0, 0.6
 * end
 * @endcode
 *
 * Use @c SomeName later on in your behaviour trees to use this @ai{ICondition}
 *
 * @par AI metatable
 * There is a metatable that you can modify by calling @ai{LUAAIRegistry::pushAIMetatable()}.
 * This metatable is applied to all @ai{AI} pointers that are forwarded to the lua functions.
 */
class LUAAIRegistry : public AIRegistry {
protected:
	lua_State* _s = nullptr;

	using LuaNodeFactory = LUATreeNode::LUATreeNodeFactory;
	typedef std::shared_ptr<LuaNodeFactory> LUATreeNodeFactoryPtr;
	typedef std::map<std::string, LUATreeNodeFactoryPtr> TreeNodeFactoryMap;

	using LuaConditionFactory = LUACondition::LUAConditionFactory;
	typedef std::shared_ptr<LuaConditionFactory> LUAConditionFactoryPtr;
	typedef std::map<std::string, LUAConditionFactoryPtr> ConditionFactoryMap;

	using LuaFilterFactory = LUAFilter::LUAFilterFactory;
	typedef std::shared_ptr<LuaFilterFactory> LUAFilterFactoryPtr;
	typedef std::map<std::string, LUAFilterFactoryPtr> FilterFactoryMap;

	using LuaSteeringFactory = movement::LUASteering::LUASteeringFactory;
	typedef std::shared_ptr<LuaSteeringFactory> LUASteeringFactoryPtr;
	typedef std::map<std::string, LUASteeringFactoryPtr> SteeringFactoryMap;

	ReadWriteLock _lock{"luaregistry"};
	TreeNodeFactoryMap _treeNodeFactories;
	ConditionFactoryMap _conditionFactories;
	FilterFactoryMap _filterFactories;
	SteeringFactoryMap _steeringFactories;

	static LUAAIRegistry* luaGetContext(lua_State * s) {
		lua_getglobal(s, "Registry");
		LUAAIRegistry* data = (LUAAIRegistry*) lua_touserdata(s, -1);
		lua_pop(s, 1);
		return data;
	}

	static LuaNodeFactory* luaGetNodeFactoryContext(lua_State * s, int n) {
		return *(LuaNodeFactory **) lua_touserdata(s, n);
	}

	static LuaConditionFactory* luaGetConditionFactoryContext(lua_State * s, int n) {
		return *(LuaConditionFactory **) lua_touserdata(s, n);
	}

	static LuaFilterFactory* luaGetFilterFactoryContext(lua_State * s, int n) {
		return *(LuaFilterFactory **) lua_touserdata(s, n);
	}

	static LuaSteeringFactory* luaGetSteeringFactoryContext(lua_State * s, int n) {
		return *(LuaSteeringFactory **) lua_touserdata(s, n);
	}

	static int luaNodeEmptyExecute(lua_State* s) {
		const LuaNodeFactory* factory = luaGetNodeFactoryContext(s, 1);
		return luaL_error(s, "There is no execute function set for node: %s", factory->type().c_str());
	}

	static int luaNodeToString(lua_State* s) {
		const LuaNodeFactory* factory = luaGetNodeFactoryContext(s, 1);
		lua_pushfstring(s, "node: %s", factory->type().c_str());
		return 1;
	}

	/**
	 * @brief Create a new lua @ai{TreeNode}
	 *
	 * @par lua parameters: #1 name of the node
	 * @note you have to specify an @c execute method that accepts two parameters in your lua code. E.g. do it like this:
	 * @code
	 * local luatest = REGISTRY.createNode(\"LuaTest\")
	 " function luatest:execute(ai, deltaMillis)
	 "    return FAILED\n"
	 " end
	 * @endcode
	 */
	static int luaCreateNode(lua_State* s) {
		LUAAIRegistry* r = luaGetContext(s);
		const std::string type = luaL_checkstring(s, -1);
		const LUATreeNodeFactoryPtr& factory = std::make_shared<LuaNodeFactory>(s, type);
		const bool inserted = r->registerNodeFactory(type, *factory);
		if (!inserted) {
			return luaL_error(s, "tree node %s is already registered", type.c_str());
		}

		lua_ainewuserdata<LuaNodeFactory>(s, factory.get());
		const luaL_Reg nodes[] = {
			{"execute", luaNodeEmptyExecute},
			{"__tostring", luaNodeToString},
			{"__newindex", lua_ainewindex},
			{nullptr, nullptr}
		};
		lua_aisetupmetatable(s, type, nodes, "node");
		ScopedWriteLock scopedLock(r->_lock);
		r->_treeNodeFactories.emplace(type, factory);
		return 1;
	}

	static int luaConditionEmptyEvaluate(lua_State* s) {
		const LuaConditionFactory* factory = luaGetConditionFactoryContext(s, 1);
		return luaL_error(s, "There is no evaluate function set for condition: %s", factory->type().c_str());
	}

	static int luaConditionToString(lua_State* s) {
		const LuaConditionFactory* factory = luaGetConditionFactoryContext(s, 1);
		lua_pushfstring(s, "condition: %s", factory->type().c_str());
		return 1;
	}

	static int luaCreateCondition(lua_State* s) {
		LUAAIRegistry* r = luaGetContext(s);
		const std::string type = luaL_checkstring(s, -1);
		const LUAConditionFactoryPtr& factory = std::make_shared<LuaConditionFactory>(s, type);
		const bool inserted = r->registerConditionFactory(type, *factory);
		if (!inserted) {
			return luaL_error(s, "condition %s is already registered", type.c_str());
		}

		lua_ainewuserdata<LuaConditionFactory>(s, factory.get());
		const luaL_Reg nodes[] = {
			{"evaluate", luaConditionEmptyEvaluate},
			{"__tostring", luaConditionToString},
			{"__newindex", lua_ainewindex},
			{nullptr, nullptr}
		};
		lua_aisetupmetatable(s, type, nodes, "condition");

		ScopedWriteLock scopedLock(r->_lock);
		r->_conditionFactories.emplace(type, factory);
		return 1;
	}

	static int luaFilterEmptyFilter(lua_State* s) {
		const LuaFilterFactory* factory = luaGetFilterFactoryContext(s, 1);
		return luaL_error(s, "There is no filter function set for filter: %s", factory->type().c_str());
	}

	static int luaFilterToString(lua_State* s) {
		const LuaFilterFactory* factory = luaGetFilterFactoryContext(s, 1);
		lua_pushfstring(s, "filter: %s", factory->type().c_str());
		return 1;
	}

	static int luaCreateFilter(lua_State* s) {
		LUAAIRegistry* r = luaGetContext(s);
		const std::string type = luaL_checkstring(s, -1);
		const LUAFilterFactoryPtr& factory = std::make_shared<LuaFilterFactory>(s, type);
		const bool inserted = r->registerFilterFactory(type, *factory);
		if (!inserted) {
			return luaL_error(s, "filter %s is already registered", type.c_str());
		}

		lua_ainewuserdata<LuaFilterFactory>(s, factory.get());
		const luaL_Reg nodes[] = {
			{"filter", luaFilterEmptyFilter},
			{"__tostring", luaFilterToString},
			{"__newindex", lua_ainewindex},
			{nullptr, nullptr}
		};
		lua_aisetupmetatable(s, type, nodes, "filter");

		ScopedWriteLock scopedLock(r->_lock);
		r->_filterFactories.emplace(type, factory);
		return 1;
	}

	static int luaSteeringEmptyExecute(lua_State* s) {
		const LuaSteeringFactory* factory = luaGetSteeringFactoryContext(s, 1);
		return luaL_error(s, "There is no execute() function set for steering: %s", factory->type().c_str());
	}

	static int luaSteeringToString(lua_State* s) {
		const LuaSteeringFactory* factory = luaGetSteeringFactoryContext(s, 1);
		lua_pushfstring(s, "steering: %s", factory->type().c_str());
		return 1;
	}

	static int luaCreateSteering(lua_State* s) {
		LUAAIRegistry* r = luaGetContext(s);
		const std::string type = luaL_checkstring(s, -1);
		const LUASteeringFactoryPtr& factory = std::make_shared<LuaSteeringFactory>(s, type);
		const bool inserted = r->registerSteeringFactory(type, *factory);
		if (!inserted) {
			return luaL_error(s, "steering %s is already registered", type.c_str());
		}

		lua_ainewuserdata<LuaSteeringFactory>(s, factory.get());
		const luaL_Reg nodes[] = {
			{"filter", luaSteeringEmptyExecute},
			{"__tostring", luaSteeringToString},
			{"__newindex", lua_ainewindex},
			{nullptr, nullptr}
		};
		lua_aisetupmetatable(s, type, nodes, "steering");

		ScopedWriteLock scopedLock(r->_lock);
		r->_steeringFactories.emplace(type, factory);
		return 1;
	}

	void registerFuncs(luaL_Reg* funcs, const char *name) {
		luaL_newmetatable(_s, name);
		// assign the metatable to __index
		lua_pushvalue(_s, -1);
		lua_setfield(_s, -2, "__index");
		luaL_setfuncs(_s, funcs, 0);
	}
public:
	LUAAIRegistry() {
		init();
	}

	/**
	 * @brief Access to the lua state.
	 * @see pushAIMetatable()
	 */
	lua_State* getLuaState() {
		return _s;
	}

	/**
	 * @brief Pushes the AI metatable onto the stack. This allows anyone to modify it
	 * to provide own functions and data that is applied to the @c ai parameters of the
	 * lua functions.
	 * @note lua_ctxai() can be used in your lua c callbacks to get access to the
	 * @ai{AI} pointer: @code const AI* ai = lua_ctxai(s, 1); @endcode
	 */
	int pushAIMetatable() {
		ai_assert(_s != nullptr, "LUA state is not yet initialized");
		return luaL_getmetatable(_s, lua_metaai());
	}

	/**
	 * @brief Pushes the character metatable onto the stack. This allows anyone to modify it
	 * to provide own functions and data that is applied to the @c ai:character() value
	 */
	int pushCharacterMetatable() {
		ai_assert(_s != nullptr, "LUA state is not yet initialized");
		return luaL_getmetatable(_s, lua_metacharacter());
	}

	/**
	 * @see shutdown()
	 */
	bool init() {
		if (_s != nullptr) {
			return true;
		}
		_s = luaL_newstate();

		lua_atpanic(_s, [] (lua_State* L) {
			ai_log_error("Lua panic. Error message: %s", (lua_isnil(L, -1) ? "" : lua_tostring(L, -1)));
			return 0;
		});
		lua_gc(_s, LUA_GCSTOP, 0);
		luaL_openlibs(_s);

		luaL_Reg registryFuncs[] = {
			{"createNode", luaCreateNode},
			{"createCondition", luaCreateCondition},
			{"createFilter", luaCreateFilter},
			{"createSteering", luaCreateSteering},
			{nullptr, nullptr}
		};
		registerFuncs(registryFuncs, "META_REGISTRY");
		lua_setglobal(_s, "REGISTRY");

		// TODO: random, vec3

		lua_pushlightuserdata(_s, this);
		lua_setglobal(_s, "Registry");

		registerAIFunc();
		registerZoneFunc();
		registerAggroMgrFunc();
		registerCharacterFunc();

		const char* script = ""
			"UNKNOWN, CANNOTEXECUTE, RUNNING, FINISHED, FAILED, EXCEPTION = 0, 1, 2, 3, 4, 5\n";

		if (luaL_loadbufferx(_s, script, strlen(script), "", nullptr) || lua_pcall(_s, 0, 0, 0)) {
			ai_log_error("%s", lua_tostring(_s, -1));
			lua_pop(_s, 1);
			return false;
		}
		return true;
	}

	/**
	 * @see init()
	 */
	void shutdown() {
		{
			ScopedWriteLock scopedLock(_lock);
			_treeNodeFactories.clear();
			_conditionFactories.clear();
			_filterFactories.clear();
			_steeringFactories.clear();
		}
		if (_s != nullptr) {
			lua_close(_s);
			_s = nullptr;
		}
	}

	~LUAAIRegistry() {
		shutdown();
	}

	inline bool evaluate(const std::string& str) {
		return evaluate(str.c_str(), str.length());
	}

	/**
	 * @brief Load your lua scripts into the lua state of the registry.
	 * This can be called multiple times to e.g. load multiple files.
	 * @return @c true if the lua script was loaded, @c false otherwise
	 * @note you have to call init() before
	 */
	bool evaluate(const char* luaBuffer, size_t size) {
		if (_s == nullptr) {
			ai_log_debug("LUA state is not yet initialized");
			return false;
		}
		if (luaL_loadbufferx(_s, luaBuffer, size, "", nullptr) || lua_pcall(_s, 0, 0, 0)) {
			ai_log_error("%s", lua_tostring(_s, -1));
			lua_pop(_s, 1);
			return false;
		}
		return true;
	}

	virtual void registerAIFunc() {
		luaL_Reg aiFuncs[] = {
			// TODO: filtered entities
			{"id", lua_aiid},
			{"time", lua_aitime},
			{"hasZone", lua_aihaszone},
			{"zone", lua_aigetzone},
			{"character", lua_aigetcharacter},
			{"aggroMgr", lua_aigetaggromgr},
			{"__tostring", lua_aitostring},
			{nullptr, nullptr}
		};
		registerFuncs(aiFuncs, lua_metaai());
	}

	virtual void registerZoneFunc() {
		luaL_Reg zoneFuncs[] = {
			{"size", lua_zonesize},
			{"__tostring", lua_zonetostring},
			{nullptr, nullptr}
		};
		registerFuncs(zoneFuncs, lua_metazone());
	}

	virtual void registerCharacterFunc() {
		luaL_Reg characterFuncs[] = {
			// TODO: position (vec3)
			{"id", lua_characterid},
			{"speed", lua_characterspeed},
			{"setSpeed", lua_charactersetspeed},
			{"orientation", lua_characterorientation},
			{"setOrientation", lua_charactersetorientation},
			{"setAttribute", lua_charactersetattribute},
			{"attributes", lua_characterattributes},
			{"__tostring", lua_charactertostring},
			{nullptr, nullptr}
		};
		registerFuncs(characterFuncs, lua_metacharacter());
	}

	virtual void registerAggroMgrFunc() {
		luaL_Reg aggroMgrFuncs[] = {
			{"addAggro", lua_aggromgraddaggro},
			{"__tostring", lua_aggromgrtostring},
			{nullptr, nullptr}
		};
		registerFuncs(aggroMgrFuncs, lua_metaaggromgr());
	}
};

}