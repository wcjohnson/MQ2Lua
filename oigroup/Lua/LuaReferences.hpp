/*
 * LuaReferences.hpp
 *
 *  Created on: Apr 20, 2012
 *      Author: wcj
 *
 *  RAII-based wrapper around Lua's reference mechanism, which allows C++ to hold on to Lua entities
 *  which would otherwise be garbage collected.
 */

#ifndef LUAREFERENCES_HPP_
#define LUAREFERENCES_HPP_

#include <lua/lua.hpp>

namespace oigroup { namespace Lua {

/// POD reference type
struct PODReference {
	lua_State * L;
	int key;
};

////////////////////////////////////////////
// LUA REFERENCES
// Importing heavyweight objects from Lua to C requires the use of Lua's reference mechanism, which
// holds copies of stuff inside of Lua's internal registry so they don't get garbage collected.
//
// This is a RAII representation of a Lua reference. Destruction or reassignment frees the underlying
// reference inside of the Lua state.
struct Reference {
	lua_State * L;
	int key;

	Reference() : L(0), key(LUA_NOREF) { }
	~Reference() { Free(); }

	// Does this reference reference anything?
	bool IsValid() const {
		return L && key != LUA_REFNIL && key != LUA_NOREF;
	}

	// Free this reference.
	void Free() {
		if(IsValid()) luaL_unref(L, LUA_REGISTRYINDEX, key);
		key = LUA_NOREF;
	}

	// Push the thing referenced by this reference onto the stack. Puts nil on the
	// stack if anything is amiss.
	bool Push(lua_State * LL) const {
		if ((!IsValid()) || (L != LL)) {
			lua_pushnil(LL); return false;
		} else {
			lua_rawgeti(L, LUA_REGISTRYINDEX, key); return true;
		}
	}

	// Pop the thing on the top of the stack into this reference. If the
	// thing on the top of the stack is nil, quashes the reference; it refers
	// to nil afterwards.
	bool Pop(lua_State * LL) {
		if(lua_isnoneornil(LL, -1)) {
			if(LL == L) { Free(); return true; }
		} else {
			Free();
			L = LL; key = luaL_ref(L, LUA_REGISTRYINDEX);
			return true;
		}
		return false;
	}
};

// A reference that does Lua type checking before storing a referent.
template <int LuaTypeID>
struct TypedReference : Reference {
	// When popping, we will verify the type before storing it.
	bool Pop(lua_State * LL) {
		int ty = lua_type(LL, -1);
		if(ty == LuaTypeID) {
			Free();
			L = LL; key = luaL_ref(L, LUA_REGISTRYINDEX);
			return true;
		} else if ( (ty == LUA_TNIL) && (LL = L) ) {
			// Setting to nil = reset
			Free();
			return true;
		}
		return false;
	}
};

typedef TypedReference<LUA_TFUNCTION> FunctionReference;
typedef TypedReference<LUA_TTABLE> TableReference;

// LuaPush specialization for references. Pushes the referent.

} } // namespace oigroup::Lua

#endif /* LUAREFERENCES_HPP_ */
