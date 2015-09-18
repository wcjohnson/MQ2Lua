/*
 * LuaPrimitives.h
 *
 *  Created on: Apr 20, 2012
 *      Author: wcj
 *
 *	Marshal and unmarshal primitive Lua data types to/from the Lua stack.
 */

/**
 * @defgroup Lua oigroup::Lua
 */

#ifndef LUAPRIMITIVES_H_
#define LUAPRIMITIVES_H_

#include <lua/lua.hpp>
#include <oigroup/Meta/EnableIf.hpp>
#include <string>
#include <type_traits>
#include <typeinfo>

#include "LuaObject.hpp"
#include "LuaReferences.hpp"

namespace oigroup { namespace Lua {

/**
 * @class LuaMarshal
 * @ingroup Lua
 * @brief Template metaprogramming code generator for marshalling between C++ and Lua.
 *
 * This class, which should be specialized for each unqualified C++ type that is meant
 * to be moved to and from Lua, either gets or pushes a Lua entity onto the Lua stack
 * that corresponds to the given C++ type.
 *
 * @tparam T The unqualified C++ type that is to be marshalled or unmarshalled from Lua.
 */
template <typename T, class Sfinae = void> struct LuaMarshal {
	/// Get a value of type T from the Lua stack at the specified index.
	/// @param[in] L The lua_State to operate on.
	/// @param[out] x A reference to an object of type T, which will be assigned the marshalled value.
	/// @return true iff the operation succeeded in assigning a marshalled value to x.
	static inline bool get(lua_State * const L, int idx, T & x);

	/// Check if the value at the specified Lua stack index can be coerced into type T. If so,
	/// get its T-like value; if not, a Lua error will be thrown.
	/// @param[in] L The lua_State to operate on.
	/// @param[out] x A reference to an object of type T, which will be assigned the marshalled value.
	static inline void check(lua_State * const L, int idx, T & x);

	/// Push a value of type T onto the top of the Lua stack.
	/// @param[in] L The lua_State to operate on
	/// @param[in] x A reference to an object of type T, which will be marshalled and pushed to the stack.
	static inline void push(lua_State * const L, T const & x);
};

// Partial specialization: bool
template <>
struct LuaMarshal<bool> {
	static inline bool get(lua_State * const L, int idx, bool & x) {
		x = static_cast<bool>(lua_toboolean(L, idx));
		return true;
	}
	static inline void check(lua_State * const L, int idx, bool & x) {
		x = static_cast<bool>(lua_toboolean(L, idx));
	}
	static inline void push(lua_State * const L, bool const & x) {
		lua_pushboolean(L, x);
	}
};

// Partial specialization: int
template <>
struct LuaMarshal<int> {
	static inline bool get(lua_State * const L, int idx, int & x) {
		int isnum;
		x = lua_tointegerx(L, idx, &isnum);
		return static_cast<bool>(isnum);
	}
	static inline void check(lua_State * const L, int idx, int & x) {
		x = static_cast<int>(luaL_checkinteger(L, idx)); // if this fails, this function won't return anyway
	}
	static inline void push(lua_State * const L, int const & x) {
		lua_pushinteger(L, x);
	}
};

// Partial specialization: float
template <>
struct LuaMarshal<float> {
	static inline bool get(lua_State * const L, int idx, float & x) {
		int isnum;
		x = static_cast<float>(lua_tonumberx(L, idx, &isnum));
		return static_cast<bool>(isnum);
	}
	static inline void check(lua_State * const L, int idx, float & x) {
		x = static_cast<float>(luaL_checknumber(L, idx));
	}
	static inline void push(lua_State * const L, float const & x) {
		lua_pushnumber(L, static_cast<lua_Number>(x));
	}
};

// Partial specialization: double
template <>
struct LuaMarshal<double> {
	static inline bool get(lua_State * const L, int idx, double & x) {
		int isnum;
		x = static_cast<double>(lua_tonumberx(L, idx, &isnum));
		return static_cast<bool>(isnum);
	}
	static inline void check(lua_State * const L, int idx, double & x) {
		x = static_cast<double>(luaL_checknumber(L, idx));
	}
	static inline void push(lua_State * const L, double const & x) {
		lua_pushnumber(L, static_cast<lua_Number>(x));
	}
};

// Partial specialization: std::string
template <>
struct LuaMarshal<std::string> {
	static inline bool get(lua_State * const L, int idx, std::string & x) {
		size_t len;
		const char *str = lua_tolstring(L, idx, &len);
		if(str) { x.assign(str, len); return true; } else return false;
	}
	static inline void check(lua_State * const L, int idx, std::string & x) {
		size_t len;
		const char *str = luaL_checklstring(L, idx, &len);
		x.assign(str, len);
	}
	static inline void push(lua_State * const L, std::string const & x) {
		lua_pushlstring(L, x.data(), x.size());
	}
};

// Partial specialization: const char *
template <>
struct LuaMarshal<const char *> {
	static inline bool get(lua_State * const L, int idx, const char * & x) {
		const char *str = lua_tolstring(L, idx, NULL);
		if (str) { x = str; return true; } else return false;
	}
	static inline void check(lua_State * const L, int idx, const char * & x) {
		x = luaL_checklstring(L, idx, NULL);
	}
	static inline void push(lua_State * const L, const char * const & x) {
		lua_pushlstring(L, x, strlen(x));
	}
};

// Partial specialization: FunctionReference
template <>
struct LuaMarshal<FunctionReference> {
	static inline bool get(lua_State * const L, int idx, FunctionReference & x) {
		// Dup the function
		lua_pushvalue(L, idx);
		// Try to pop it
		if (x.Pop(L)) {
			return true;
		} else {
			// The dupped function is still on the stack. get rid of it.
			lua_pop(L, 1);
			return false;
		}
	}
	static inline void check(lua_State * const L, int idx, FunctionReference & x) {
		// Verify type
		luaL_checktype(L, idx, LUA_TFUNCTION);
		// Dup the function
		lua_pushvalue(L, idx);
		// Ref it
		x.Pop(L);
	}
	static inline void push(lua_State * const L, FunctionReference const & x) {
		x.Push(L);
	}
};

	template <class T>
	struct LuaMarshal<
		T *,
		typename EnableIf<std::is_base_of<LuaObject, T>::value>::type
	>
	{
		static_assert(std::is_base_of<LuaObject, T>::value, "LuaMarshal: EnableIf failed for LuaObject base class");
		
		static inline bool get(lua_State * const L, int n, T*& x) {
			// The object at index n should be a table...
			if(lua_type(L, n) != LUA_TTABLE) return 0;
			// Get element [0] of this table...
			lua_rawgeti(L, n, 0);
			// Either lightuserdata (directly dynamically_castable to LuaObject) or
			// heavy userdata (needs to be unwrapped)
			int ty = lua_type(L, -1);
			if(ty == LUA_TLIGHTUSERDATA) {
				// A lightuserdata ought to be dynamic_castable to T.
				x = dynamic_cast<T*>(static_cast<LuaObject *>(lua_touserdata(L, -1)));
				lua_pop(L, 1);
				return (x != 0);
			} else if(ty == LUA_TUSERDATA) {
				LuaUserdata * ud = static_cast<LuaUserdata *>(lua_touserdata(L, -1));
				lua_pop(L, 1);
				x = dynamic_cast<T *>(ud->unwrap());
				return (x != 0);
			} else {
				lua_pop(L, 1);
				return false;
			}
		}
		
		static inline void check(lua_State * const L, int n, T*& x) {
			if(!get(L, n, x)) {
				std::string s("expected a C object of type "); s += typeid(T).name();
				luaL_argerror(L, n, s.c_str());
			}
		}
		
		static inline void push(lua_State * const L, T* const & x) {
			(static_cast<LuaObject *>(x))->LuaPush(L);
		}
	};

/// Push a value onto the Lua stack using LuaMarshal with the inferred type.
/// @param[in] L The lua_State whose stack onto which to push the value.
/// @param[in] x Reference to the value to be pushed.
template <typename T>
inline void LuaPush(lua_State * const L, T const & x) { LuaMarshal<T>::push(L, x); }

/// Get a value from the Lua stack using LuaMarshal with the inferred type.
/// @param[in] L The lua_State whose stack from which to get the value.
/// @param[in] idx The position on the stack to get from.
/// @param[out] x Reference to a T to which the received value will be assigned.
/// @return true iff a T was successfully assigned to x. If false is returned, the state of x is
/// indeterminate.
template <typename T>
inline bool LuaGet(lua_State * const L, int idx, T & x) { return LuaMarshal<T>::get(L, idx, x); }

template <typename T>
inline void LuaGetOrDefault(lua_State * const L, int idx, T & x, T const & def) {
	if(!LuaMarshal<T>::get(L, idx, x)) x = def;
}

/// Get a value from the Lua stack using LuaMarshal with the inferred type, throwing a Lua
/// error if the value on the stack does not match the type.
/// @param[in] L The lua_State whose stack from which to get the value.
/// @param[in] idx The position on the stack to get from.
/// @param[out] x Reference to a T to which the received value will be assigned.
template <typename T>
inline void LuaCheck(lua_State * const L, int idx, T & x) { LuaMarshal<T>::check(L, idx, x); }


// Base case
template <typename...> void LuaVariadicPush(lua_State * const L) { }

/// Push multiple values onto the Lua stack variadically.
/// @tparam Args... Inferred type of the passed arguments.
/// @param[in] L The lua_State onto whose stack to push the values.
/// @param[in] args... The values to be pushed onto the stack, in direct order.
template <typename Arg, typename... Args>
void LuaVariadicPush(lua_State * const L, Arg const & arg, Args const &... args) {
	LuaMarshal<Arg>::push(L, arg);
	LuaVariadicPush<Args...>(L, args...);
}

////////////////////////////////////////////
// IDIOMATIC CONSTRUCTS
// These are some syntactic sugar for typical Lua interactions.


/// Check that there are n arguments on the Lua stack; raise a Lua error if there
/// are not.
/// @param[in] L the lua_State to be examined
/// @param[in] n The number of arguments that must be matched.
inline void LuaCheckNArgs(lua_State * const L, int n) {
	int got = lua_gettop(L);
	if(got != n) luaL_error(L, "expected %d args, got %d.", n, got);
}

/// Discard entries from the top of the Lua stack, e.g. unused return values from a
/// Lua function. Idiomatically, this can be used with the LuaCall... functions like so:
/// 	LuaDiscard(L, LuaVariadicCall(L, args...));
/// which would invoke a function and discard all of its results.
///
/// @param[in] L The lua_State whose stack will be popped
/// @param[in] how_many The number of entries to pop.
inline void LuaDiscard(lua_State * const L, int how_many) {
	if(how_many > 0) lua_settop(L, lua_gettop(L) - how_many);
}



} } // namespace oigroup::Lua

#endif /* LUAPRIMITIVES_H_ */
