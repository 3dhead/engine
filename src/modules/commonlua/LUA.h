/**
 * @file
 */

#pragma once
extern "C" {
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>
}
#include <string>
#include <map>
#include <memory>

#include "core/Log.h"
#include "core/NonCopyable.h"

namespace lua {

const std::string META_PREFIX = "META_";

class LUAType {
private:
	lua_State* _state;
public:
	LUAType(lua_State* state, const std::string& name);

	// only non-capturing lambdas can be converted to function pointers
	template<class FUNC>
	void addFunction(const std::string& name, FUNC&& func) {
		lua_pushcfunction(_state, func);
		lua_setfield(_state, -2, name.c_str());
	}
};

class LUA : public core::NonCopyable {
private:
	lua_State *_state;
	std::string _error;
	bool _destroy;

public:
	explicit LUA(lua_State *state);

	explicit LUA(bool debug = false);
	~LUA();

	lua_State* state() const;

	template<class T>
	static T* newGlobalData(lua_State *L, const std::string& prefix, T *userData) {
		lua_pushlightuserdata(L, userData);
		lua_setglobal(L, prefix.c_str());
		return userData;
	}

	template<class T>
	inline T* newGlobalData(const std::string& prefix, T *userData) const {
		newGlobalData(_state, prefix, userData);
		return userData;
	}

	template<class T>
	static T* globalData(lua_State *L, const std::string& prefix) {
		lua_getglobal(L, prefix.c_str());
		T* data = (T*) lua_touserdata(L, -1);
		lua_pop(L, 1);
		return data;
	}

	template<class T>
	inline T* globalData(const std::string& prefix) const {
		return globalData<T>(_state, prefix);
	}

	template<class FUNC>
	inline void registerGlobal(const char* name, FUNC&& f) const {
		lua_pushcfunction(_state, f);
		lua_setglobal(_state, name);
	}

	template<class T>
	static T** newUserdata(lua_State *L, const std::string& prefix) {
		T ** udata = (T **) lua_newuserdata(L, sizeof(T *));
		const std::string name = META_PREFIX + prefix;
		luaL_getmetatable(L, name.c_str());
		lua_setmetatable(L, -2);
		return udata;
	}

	template<class T>
	static T* newUserdata(lua_State *L, const std::string& prefix, T* data) {
		T ** udata = (T **) lua_newuserdata(L, sizeof(T *));
		const std::string name = META_PREFIX + prefix;
		luaL_getmetatable(L, name.c_str());
		lua_setmetatable(L, -2);
		*udata = data;
		return data;
	}

	template<class T>
	static T* userData(lua_State *L, int n, const std::string& prefix) {
		const std::string name = META_PREFIX + prefix;
		return *(T **) luaL_checkudata(L, n, name.c_str());
	}

	/**
	 * Aborts the lua execution with the given error message
	 */
	static int returnError(lua_State *L, const std::string& error) {
		Log::error("LUA error: %s", error.c_str());
		return luaL_error(L, "%s", error.c_str());
	}

	void global(const std::string& name);

	std::string key();

	void globalKeyValue(const std::string& name);

	bool nextKeyValue();

	void pop(int amount = 1);

	int table(const std::string& name);

	std::string tableString(int i);

	int tableInteger(int i);

	float tableFloat(int i);

	void reg(const std::string& prefix, const luaL_Reg* funcs);
	LUAType registerType(const std::string& name);

	void setError(const std::string& error);
	const std::string& error() const;
	/**
	 * @brief Loads a lua script into the lua state.
	 */
	bool load(const std::string &luaString);
	/**
	 * @brief Executes a function from an already loaded lua state
	 * @param[in] function function to be called
	 * @param[in] returnValues The amount of values returned by the called lua function. -1 is for multiple return values.
	 * @note Use clua_get<T>(s, -1) to get the first custom return value.
	 */
	bool execute(const std::string &function, int returnValues = 0);

	std::string valueStringFromTable(const char * key, const std::string& defaultValue = "");
	float valueFloatFromTable(const char * key, float defaultValue = 0.0f);
	int valueIntegerFromTable(const char * key, int defaultValue = 0);
	bool valueBoolFromTable(const char * key, bool defaultValue = false);
	void keyValueMap(std::map<std::string, std::string>& map, const char *key);

	int intValue(const std::string& path, int defaultValue = 0);
	float floatValue(const std::string& path, float defaultValue = 0.0f);
	std::string stringFromStack();
	std::string string(const std::string& expr, const std::string& defaultValue = "");

	static std::string stackDump(lua_State *L);
	std::string stackDump();
};

inline lua_State* LUA::state() const {
	return _state;
}

inline void LUA::setError(const std::string& error) {
	_error = error;
}

inline const std::string& LUA::error() const {
	return _error;
}

typedef std::shared_ptr<LUA> LUAPtr;

}
