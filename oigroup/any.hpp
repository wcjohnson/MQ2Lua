/*
 * any.hpp
 *
 *  Created on: May 9, 2012
 *      Author: wcj
 */

#ifndef ANY_HPP_
#define ANY_HPP_

class any {
public:
	any() : content(0) { }
	template <typename T> any(const T & other) : content(new t_holder<T>(other)) { }
	any (const any & other) : content(other.content ? other.content->clone() : 0) { }
	~any() { delete content; }

	bool empty() const { return !content; }
	const std::type_info & type() const { return content ? content->type() : typeid(void); }

protected:
	class holder {
	public:
		virtual ~holder() { }
		virtual const std::type_info & type() const = 0;
		virtual holder * clone() const = 0;
	};

	holder * content;

	template <typename T>
	class t_holder : public holder {
	public:
		T obj;
		t_holder(const T & what) : obj(what) { }
		virtual const std::type_info & type() const { return typeid(T); }
		virtual holder * clone() const { return new t_holder(obj); }
	private:
		t_holder(const t_holder &);
		t_holder & operator =(const t_holder &);
	};


};


#endif /* ANY_HPP_ */
