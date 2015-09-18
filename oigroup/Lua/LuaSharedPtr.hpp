/*
 * LuaSharedPtr.hpp
 *
 *  Created on: Aug 11, 2012
 *      Author: wcj
 */

#ifndef LUASHAREDPTR_HPP_
#define LUASHAREDPTR_HPP_

#include "LuaObject.hpp"
#include <memory>

namespace oigroup { namespace Lua {

/**
 * @brief A heavy LuaUserdata wrapping a std::shared_ptr, releasing the refcount upon garbage collection
 */
template <class T>
class LuaSharedPtr : public LuaUserdata {
protected:
	std::shared_ptr<T> ptr;

public:
	LuaSharedPtr(std::shared_ptr<T> const & it) : ptr(it) { }
	LuaSharedPtr(std::shared_ptr<T> && it) : ptr(it) { }
	virtual ~LuaSharedPtr() { }
	virtual LuaObject * unwrap() { return ptr.get(); }
};

} } // namespace oigroup::Lua


#endif /* LUASHAREDPTR_HPP_ */
