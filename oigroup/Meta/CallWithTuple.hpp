/*
 * CallWithTuple.h
 *
 *  Created on: Apr 18, 2012
 *      Author: wcj
 *
 *  Call callable objects using tuple arguments.
 */
#ifndef CALLWITHTUPLE_HPP_
#define CALLWITHTUPLE_HPP_

#include <tuple>
#include <utility>

#include "IntSeqPack.hpp"

namespace oigroup {

template <typename TupleT, typename Ret, typename... Args, int... Indices >
inline Ret _CallWithTuple( Ret(*pfn)(Args...), TupleT & t, IntegerSequencePack<Indices...>) {
	return (*pfn)(std::forward<Args>(std::get<Indices>(t))...);
}

template <typename TupleT, typename Ret, typename... Args>
inline Ret CallWithTuple( Ret (*pfn)(Args...), TupleT & t ) {
	return _CallWithTuple(pfn, t, typename GenerateIntegerSequencePack<sizeof...(Args)>::type());
}

} // namespace oigroup

#endif
