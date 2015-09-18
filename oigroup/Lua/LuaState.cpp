/*
 * LuaState.cpp
 *
 *  Created on: May 1, 2012
 *      Author: wcj
 */


#include "LuaState.hpp"
#include <cstring>
#include "LuaStackMarker.hpp"
#include "LuaException.hpp"

using namespace oigroup::Lua;
using namespace std;

// Bizarrely, the libg++ version of std::vector has some sort of static initialization order problem
// that causes its _m_end pointer to get corrupted when it's used during static. Hence this stupid thing.
struct InitArray {
	LuaState::InitFunc * array;
	int sz;

	InitArray() : array(0), sz(0) { }
	~InitArray() { delete [] array; }

	void insert(LuaState::InitFunc fn) {
		// Move everything to a newArray
		sz++;
		LuaState::InitFunc * newArray = new LuaState::InitFunc[sz];
		memcpy(newArray, array, (sz-1)*sizeof(LuaState::InitFunc));
		newArray[sz - 1] = fn;
		// Delete the old array
		delete [] array;
		// Replace.
		array = newArray;
	}
};


// Singleton vector of state initialization functions
InitArray & GetInitFuncs() {
	static InitArray s;
	return s;
}

void LuaState::RegisterGlobalInitFunc(InitFunc fn) {
	GetInitFuncs().insert(fn);
}

LuaState::LuaState() : L(0) {
	L = luaL_newstate();
	if(L == 0) throw LuaException().append_msg("luaL_newstate failed.");
	this->init();
}

LuaState::LuaState(lua_State * _L) : L(_L) {
	if(L == 0) throw LuaException().append_msg("LuaState must be initialized with a non-null Lua state.");
	this->init();
}

LuaState::~LuaState() {
	destroy();
}

void LuaState::destroy() {
	if(L) { lua_close(L); L = 0; }
}

void LuaState::init() {
	// Load the Lua base libs
	InstallGlobalLibrary("_G", luaopen_base);
	InstallGlobalLibrary("table", luaopen_table);
	InstallGlobalLibrary("string", luaopen_string);
	InstallGlobalLibrary("package", luaopen_package);
	InstallGlobalLibrary("math", luaopen_math);
	InstallGlobalLibrary("bit32", luaopen_bit32);
	// Create oigroup.stateflags registry entry
	{
		LuaStackMarker sm(L);
		lua_pushstring(L, "oigroup.stateflags");
		lua_newtable(L);
		lua_rawset(L, LUA_REGISTRYINDEX);
	}
	// Run all the global init funcs on this state.
	InitArray & s = GetInitFuncs();
	for(int i=0; i<s.sz; ++i) {
		(s.array[i])(*this);
	}
}

bool LuaState::getFlag(char const * name) {
	LuaStackMarker sm(L);
	lua_getfield(L, LUA_REGISTRYINDEX, "oigroup.stateflags");
	lua_pushstring(L, name);
	lua_rawget(L, -2);
	return static_cast<bool>(lua_toboolean(L, -1));
}

bool LuaState::loadString(const char * code, bool leaveError)
{
	if (luaL_loadstring(L, code)) {
		// Error
		if (leaveError)
			return false;
		else {
			lua_pop(L, 1);
			lua_pushnil(L);
			return false;
		}
	} else {
		return true;
	}
}

bool LuaState::eval(const char * code)
{
	// Load the chunk.
	if (!loadString(code, true)) return false;
	if (lua_pcall(L, 0, 0, 0))
		return false;
	else
		return true;
}

bool oigroup::Lua::LuaState::evalAndGetError(const char * code, std::string & errmsg)
{
	// Load the chunk.
	if ((!loadString(code, true)) || (lua_pcall(L, 0, 0, 0))) {
		LuaGet(L, 1, errmsg);
		lua_pop(L, 1);
		return false;
	} else {
		return true;
	}
}

void LuaState::setFlag(char const * name, bool value) {
	LuaStackMarker sm(L);
	lua_getfield(L, LUA_REGISTRYINDEX, "oigroup.stateflags");
	lua_pushstring(L, name);
	if(value) lua_pushboolean(L, true); else lua_pushnil(L);
	lua_rawset(L, -3);
}

void LuaState::InstallGlobalLibrary(char const * name, lua_CFunction loader) {
	luaL_requiref(L, name, loader, 1);
	lua_pop(L, 1);
}

void LuaState::InstallPreloader(char const * name, lua_CFunction loader) {
	LuaStackMarker sm(L);
	luaL_getsubtable(L, LUA_REGISTRYINDEX, "_PRELOAD");
	lua_pushcfunction(L, loader);
	lua_setfield(L, -2, name);
}

void LuaState::RunPreloader(char const * name) {
	LuaStackMarker sm(L);
	luaL_getsubtable(L, LUA_REGISTRYINDEX, "_PRELOAD");
	lua_getfield(L, -1, name);
	lua_CFunction f = lua_tocfunction(L, -1);
	if(f) {
		luaL_requiref(L, name, f, 0);
	}
}

bool LuaState::Require(char const * name, bool leaveError) {
	return LuaState::RequireL(L, name, leaveError);
}

bool LuaState::RequireL(lua_State * _L, char const *name, bool leaveError) {
	lua_getglobal(_L, "require");
	lua_pushstring(_L, name);
	if(lua_pcall(_L, 1, 1, 0) != LUA_OK) {
		if(leaveError)
			return false;
		else {
			lua_pop(_L, 1);
			lua_pushnil(_L);
			return false;
		}
	} else {
		return true;
	}
}

void LuaState::SetPackagePath(const char *where) {
	LuaStackMarker sm(L);
	// _G["package"]
	lua_getglobal(L, "package");
	// _G["package"]/where
	lua_pushstring(L, where);
	// _G["package"]["path"] = where
	lua_setfield(L, -2, "path");
}