/*
 * LuaFunctional.h
 *
 *  Created on: Apr 20, 2012
 *      Author: wcj
 *
 *  Invoke Lua functions from C++ and C++ functions from Lua.
 */

#ifndef LUAFUNCTIONAL_H_
#define LUAFUNCTIONAL_H_

#include <type_traits>

#include "LuaTuples.hpp"
#include <oigroup/Meta/CallWithTuple.hpp>

namespace oigroup { namespace Lua {

/////////////////////////////////////////////////////////////////////////
// AUTOMATED C STUB GENERATION
//
// Allow arbitrary C functions, known at compile time, to be statically wired to a
// stub of lua_CFunction type that will marshall and unmarshall the args and returns.
// We accomplish this by generating compile-time stubs keyed on the address of the
// target function as a non-type template parameter. The rest is handled by the LuaTuples interface.
//
// The macro LuaStaticFunctionStub() provides the syntactic sugar that ties it all together. So
// for instance, you can register a Lua function that calls a static function in your code with:
//		lua_register(L, "MyFunction", LuaStaticFunctionStub(&MyFunction));
//
// It's that simple.

// General case
template <typename Ret, typename... Args>
struct __cfn_stub {
	template < Ret (*Pfn)(Args...) >
	static int stub(lua_State * ls) {
		std::tuple< typename std::remove_cv<typename std::remove_reference<Args>::type>::type ... > tup;
		LuaGetTuple(ls, tup);
		LuaMarshal<Ret>::push(ls, oigroup::CallWithTuple(Pfn, tup));
		return 1;
	}
};

// Void specialization
template <typename... Args>
struct __cfn_stub<void, Args...> {
	template < void (*Pfn)(Args...) >
	static int stub(lua_State * ls) {
		std::tuple< typename std::remove_cv<typename std::remove_reference<Args>::type>::type ... > tup;
		LuaGetTuple(ls, tup);
		oigroup::CallWithTuple(Pfn, tup);
		return 0;
	}
};

// This crazy nonsense allows us to force template argument inference via decltype. Otherwise it would
// not be possible to create stubs without very ugly syntax.
template <typename Ret, typename... Args>
__cfn_stub<Ret, Args...> __cfn_stub_type_inf( Ret (*)(Args...) ) {
	return __cfn_stub<Ret, Args...>();
}

// And this macro ties it all together. The argument must be a function ptr that is statically resolvable
// at compile time, as you will find out if you try to pass something that isn't!
#define LuaStaticFunctionStub(F) (decltype(::oigroup::Lua::__cfn_stub_type_inf((F)))::stub<(F)>)

//////////////////////////////////////////////////////////////////////////
// VARIADIC LUA CALLS
// Call Lua functions with variadic parameter lists. The Lua function must be on the stack already,
// just as with lua_call. Any return values are left on the stack. The number of values returned by
// Lua is given as the return value of this function.
template <typename... Args>
int LuaVariadicCall(lua_State * const L, Args const &... args) {
	int stack_top = lua_gettop(L); // at least 1, since the function is on the stack
	LuaVariadicPush<Args...>(L, args...);
	lua_call(L, sizeof...(Args), LUA_MULTRET);
	return (lua_gettop(L) - stack_top + 1);
}

} } // namespace oigroup::Lua

#endif /* LUAFUNCTIONAL_H_ */
