/*
 * ShortStringLookup.hpp
 *
 *  Created on: Aug 11, 2012
 *      Author: wcj
 */

#ifndef SHORTSTRINGLOOKUP_HPP_
#define SHORTSTRINGLOOKUP_HPP_

#include <cstring>

namespace oigroup {

/**
 * @brief A table mapping short string keys to arbitrary values.
 */
template <typename Res>
struct ShortStringLookupTable {
	char const * key;
	Res value;
};

/**
 * Look up a value by key in a ShortStringLookupTable.
 */
template <typename Res>
inline Res ShortStringLookup(ShortStringLookupTable<Res> const * const tbl, char const * what) {
	auto p = tbl;
	for(; p->key != 0; ++p) {
		if(std::strcmp(p->key, what) == 0) return p->value;
	}
	return p->value;
}

} // namespace oigroup

#endif /* SHORTSTRINGLOOKUP_HPP_ */
