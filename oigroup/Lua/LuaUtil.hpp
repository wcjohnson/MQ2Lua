/*
 * LuaUtil.hpp
 *
 *  Created on: Apr 29, 2012
 *      Author: wcj
 */

#ifndef LUAUTIL_HPP_
#define LUAUTIL_HPP_

#include <lua/lua.hpp>
#include "LuaException.hpp"

namespace oigroup { namespace Lua {

/// Compile a static chunk of Lua code and run the chunk as a function, leaving any results
/// on the stack. Throws LuaException on failure
void EvaluateChunk(lua_State * L, const char * sectionName, const char * code);

} } // namespace oigroup::Lua


#endif /* LUAUTIL_HPP_ */
