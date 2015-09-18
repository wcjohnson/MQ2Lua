/*
 * Invocator.hpp
 *
 *  Created on: Apr 18, 2012
 *      Author: wcj
 *
 * An Invocator is a metatype that represents a this pointer on which a mfn can be called.
 * It can hold regular pointers, references, smart pointers, const pointers, and so forth.
 */

#ifndef INVOCATOR_HPP_
#define INVOCATOR_HPP_

#include <type_traits>

namespace oigroup {

// Determine if an object of type Invocator could be used to invoke member functions of the
// type Base, and if so, how best to do that.
//
// The cases of interest:
// 1) Invocator * is convertible to Class *. This means that Invocator is a Class or Class&, so
//    we can call with (invocator).*(mfn)(...).
// 2) Invocator * is not convertible to Class *.
//		2a) Invocator * is a Class **, i.e. Invocator is a Class *.
//    2b) Invocator is a smart_ptr<Class>.
//				Both of these cases have the common property that add_pointer<decltype(*Invocator)> is
//				convertible to Class *.
//				We can call these both with ((*invocator).*(mfn))(...)

// Disambiguate between case 1 and not-case-1
template <typename Base, typename T>
struct _Case1 {
	typedef typename std::add_pointer<T>::type TPtr;
	typedef typename std::add_pointer<Base>::type BasePtr;


};

template <typename Base, typename Invocator>
struct InvocationInfo {
	// Remove the outer reference qualifiers from the invocator
	typedef typename std::remove_reference<Invocator>::type NoRefInvocator;
	// Now the invocator ought to be either Class [const], Class [const] * [const], or
	// 	[const] smart_ptr<[const] Class>.
	// Remove a layer of CV.
	typedef typename std::remove_cv<NoRefInvocator>::type NoCVNoRefInvocator;
	// Now we ought to have either Class, Class [const] *, or smart_ptr<[const] Class>.

	typedef typename std::remove_cv<NoRefNoCVInvocator>::type NoCVNoRefNoCVInvocator;
	// Now we ought to have either Class, Class [const] *, or smart_ptr<[const] Class>.
	// Add a pointer.
	// Check for const-ness of the underlying object
	typedef typename std::conditional<std::is_const<NoRefNoCVInvocator>::value, std::true_type, std::false_type>::type isInvocatorConst;
	// Remove


	typedef typename std::add_pointer<NoRefNoCVInvocator>::type InvocatorPtr;
	typedef typename std::add_pointer<Base>::type BasePtr;
};

}




#endif /* INVOCATOR_HPP_ */
