/*
 * LuaUtil.cpp
 *
 *  Created on: Apr 29, 2012
 *      Author: wcj
 */

#include "LuaUtil.hpp"

#include <cstring>

using namespace oigroup::Lua;
using namespace std;

void oigroup::Lua::EvaluateChunk(lua_State * L, const char * chunkName, const char * code) {
	if(luaL_loadbuffer(L, code, std::strlen(code), chunkName) != LUA_OK) {
		throw LuaException().append_msg("compilation error in chunk ").append_msg(chunkName).append_msg(": ").append_from_stack(L, -1);
	}
	if(lua_pcall(L, 0, LUA_MULTRET, 0) != LUA_OK) {
		throw LuaException().append_msg("runtime error in chunk ").append_msg(chunkName).append_msg(": ").append_from_stack(L, -1);
	}
}
