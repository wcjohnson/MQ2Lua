/*
 * LuaException.hpp
 *
 *  Created on: Aug 11, 2012
 *      Author: wcj
 */

#ifndef LUAEXCEPTION_HPP_
#define LUAEXCEPTION_HPP_

#include <exception>
#include <string>
#include <lua/lua.hpp>

namespace oigroup { namespace Lua {

/**
 * @brief A std::exception derivative used by the Lua binding.
 */
class LuaException : public std::exception {
public:
	typedef std::exception super;

	std::string _what;

	LuaException() : super(), _what() { }

	LuaException & append_msg(const char * msg) { _what += msg; return *this; }
	LuaException & append_from_stack(lua_State * L, int idx) {
		const char *it = lua_tolstring(L, idx, 0);
		if(it) _what += it;
		lua_remove(L, idx);
		return *this;
	}

	const char * what() const noexcept { return _what.c_str(); }
};

} } // namespace oigroup::Lua

#endif /* LUAEXCEPTION_HPP_ */
