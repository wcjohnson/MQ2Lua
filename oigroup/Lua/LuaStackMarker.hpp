/*
 * LuaStackMarker.hpp
 *
 *  Created on: Aug 11, 2012
 *      Author: wcj
 */

#ifndef LUASTACKMARKER_HPP_
#define LUASTACKMARKER_HPP_

#include <lua/lua.hpp>

namespace oigroup { namespace Lua {

/**
 * @ingroup Lua
 * @brief Provide an RAII guard to restore the Lua stack state in the event of an error.
 *
 * When an instance of this class is created, a marker on the Lua stack of the given
 * interpreter is stored; when destroyed, the interpreter is restored to the last stored state.
 * Create these on the C++ stack to ensure stack-neutrality when manipulating the Lua
 * stack even in the face of possible errors/exceptions. Since Lua's error handling uses
 * exceptions when compiling under C++, this will gracefully handle those errors as well.
 */
struct LuaStackMarker {
	lua_State * L;
	int pos;

	/// Store the current state of the Lua stack, which will be restored when
	/// the marker is destroyed.
	/// @param L0 The Lua state to be manipulated
	/// @param d An optional number of entries to be left atop the stack unmodified.
	inline explicit LuaStackMarker(lua_State * L0, int d = 0) : L(L0), pos(0) {
		if(L0) pos = lua_gettop(L0) - d;
	}

	/// Restore the stack to the stored position.
	~LuaStackMarker() { if(L) lua_settop(L, pos); }

	/// Reset the stored position to the new stack top.
	inline void reset(int d = 0) { if(L) pos = lua_gettop(L) - d; }
	/// Abandon this stack marker; the stack will not be modified when it is destructed.
	inline void abandon() { L = 0; }
	/// Pop the stack to where this marker was, and abandon the marker.
	inline void pop() { if(L) { lua_settop(L, pos); L=0; } }
};


} } // namespace oigroup::Lua


#endif /* LUASTACKMARKER_HPP_ */
