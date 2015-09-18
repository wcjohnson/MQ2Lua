/*
 * IntSeq.h
 *
 *  Created on: Apr 18, 2012
 *      Author: wcj
 *
 * Generates an integer sequence as a template parameter pack.
 */
#ifndef INTSEQPACK_HPP_
#define INTSEQPACK_HPP_

namespace oigroup {

// Trivial type of the resulting pack.
template <int...> struct IntegerSequencePack { };

// Recursive phase of the pack generation; appends N-1 to the parameter pack.
template <int N, int... S>
struct GenerateIntegerSequencePack : GenerateIntegerSequencePack<N-1, N-1, S...> { };

// Base case of the pack generation; when N=0, S is the sequence 0,...,N-1, which is
// exported as the IntegerSequencePack type.
template <int... S>
struct GenerateIntegerSequencePack<0, S...> { typedef IntegerSequencePack<S...> type; };

} // namespace oigroup

#endif
