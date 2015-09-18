/*
 * LuaTuples.hpp
 *
 *  Created on: Apr 20, 2012
 *      Author: wcj
 *
 *  Marshalling and unmarshalling of the Lua stack using std::tuple.
 *
 */

#ifndef LUATUPLES_HPP_
#define LUATUPLES_HPP_

#include <tuple>
#include "LuaMarshal.hpp"

namespace oigroup { namespace Lua {

////////////////////////////////////////////////////
// GET TUPLES FROM LUA
//
// Retrieve sequential variadically-specified arguments from the Lua stack and store them in a
// std::tuple.

// Base case
template <typename TupleT, int> void __get_tuple(lua_State * const ls, TupleT & tup) { }
// Inductive case
template <typename TupleT, int n, typename Arg, typename... Args>
void __get_tuple(lua_State * const ls, TupleT & tup) {
	LuaMarshal<Arg>::get(ls, n+1, std::get<n>(tup));
	__get_tuple<TupleT, n+1, Args...>(ls, tup);
}

// Retrieve a tuple. Lua type mismatches will not throw Lua errors.
template <typename... Args>
void GetTupleUnchecked(lua_State * const ls, std::tuple<Args...> & tup) {
	__get_tuple<std::tuple<Args...>, 0, Args...>(ls, tup);
}

// Retrieve a tuple. Lua type mismatches will throw Lua errors.
template <typename... Args>
void GetTupleChecked(lua_State * const ls, std::tuple<Args...> & tup) {
	__get_tuple< std::tuple<Args...>, 0, Args...>(ls, tup);
}

template <typename... Args>
void GetTuple(lua_State * const ls, std::tuple<Args...> & tup) {
	__get_tuple<std::tuple<Args...>, 0, Args...>(ls, tup);
}

////////////////////////////////////////////////////
// PUSH TUPLES TO LUA
//
// Push the entries from a std::tuple sequentially onto the Lua stack.

// Base case
template <typename TupleT, int, typename...> void __push_tuple(lua_State * const ls, TupleT & tup) { }

// Inductive case. We have to abstract away the tuple type because the induction peels off types from
// the arg pack.
template <typename TupleT, int n, typename Arg, typename... Args>
void __push_tuple(lua_State * const ls, TupleT & tup) {
	LuaMarshal<Arg>::push(ls, std::get<n>(tup));
	__push_tuple<TupleT, n+1, Args...>(ls, tup);
}

// Push a tuple onto the Lua stack in sequence.
template <typename... Args>
void PushTuple(lua_State * const ls, std::tuple<Args...> & tup) {
	__push_tuple<std::tuple<Args...>, 0, Args...>(ls, tup);
}

} } // namespace oigroup::Lua

#endif /* LUATUPLES_HPP_ */
