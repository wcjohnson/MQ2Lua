/*
 * LuaObject.hpp
 *
 *  Created on: May 7, 2012
 *      Author: wcj
 */

#ifndef LUAOBJECT_HPP_
#define LUAOBJECT_HPP_

#include <lua/lua.hpp>

namespace oigroup { namespace Lua {

/**
 * @ingroup Lua
 * @brief A polymorphic base class for all objects that can be pushed to Lua.
 */
class LuaObject {
public:
	LuaObject() { }
	virtual ~LuaObject() { }
	virtual void LuaPush(lua_State * L) { lua_pushnil(L); }
};

/**
 * @ingroup Lua
 * @brief A lightweight polymorphic wrapper meant to be used as Lua heavy userdata.
 *
 * All entities used as Lua heavy userdata inherit from this interface. When Lua garbage
 * collects the associated userdata, the virtual destructor will be called. If you intend
 * for your class to be used directly as heavy userdata in Lua, it must inherit from this
 * interface. You DO NOT need to inherit from this interface in order to access your class
 * via a shared_ptr or light userdata.
 */
class LuaUserdata : public LuaObject {
public:
	LuaUserdata() { }

	/// The destructor is called when Lua garbage-collects the underlying heavy userdata.
	virtual ~LuaUserdata() { }

	/// Called polymorphically on this object by the Lua object marshalling system. Should
	/// return a polymorphic C++ pointer to whatever object the userdata is meant to hold, or
	/// 0 on failure. By default, returns identically this.
	virtual LuaObject * unwrap() { return this; }
};

} } // namespace oigroup::Lua

#endif /* LUAOBJECT_HPP_ */
