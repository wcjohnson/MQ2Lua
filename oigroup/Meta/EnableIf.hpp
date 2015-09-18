/*
 * EnableIf.hpp
 *
 *  Created on: Aug 11, 2012
 *      Author: wcj
 */

#ifndef ENABLEIF_HPP_
#define ENABLEIF_HPP_

namespace oigroup {

	/**
	 * A heavily simplified version of boost's enable_if.
	 */
	template <bool B, class T = void>
	struct EnableIf {
		typedef T type;
	};

	template <class T>
	struct EnableIf<false, T> {};

}


#endif /* ENABLEIF_HPP_ */
