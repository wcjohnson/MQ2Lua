/*
 * LuaState.hpp
 *
 *  Created on: May 1, 2012
 *      Author: wcj
 */

#ifndef LUASTATE_HPP_
#define LUASTATE_HPP_

#include <lua/lua.hpp>
#include <oigroup/Lua/LuaMarshal.hpp>

namespace oigroup { namespace Lua {

/// Representation of a lua_State.
class LuaState {
protected:
	lua_State * L;

public:
	/// The type of an initialization function for a Lua state.
	typedef void (*InitFunc)(LuaState &);
	
	/// Register an intialization function that will be called on every LuaState when it is created.
	static void RegisterGlobalInitFunc( InitFunc pfn );
	
	//////////////////////////// L-type functions that operate on a lua_State* rather than a LuaState
	
	/// Runs Lua's require() command with the given string parameter, pushing the
	/// result. If the third argument is true, will leave any error that occurs
	/// on the stack.
	static bool RequireL(lua_State * _L, const char * name, bool leaveError = false);
	

public:
	/////////////////////// Lifecycle
	
	/// Create the internal lua_State with default allocator semantics (using luaL_newstate)
	LuaState();
	/// Wrap the given manually created lua_State. Note: init() will be run on this state!
	LuaState(lua_State * _L);
	
	virtual ~LuaState();

	/// Destroy the wrapped state and clear it.
	void destroy();
	/// Run initialization routines on this state.
	virtual void init();
	/// Get the wrapped lua_State.
	inline lua_State * getState() { return L; }
	/// Implicitly convert to lua_State *, allowing this to be provided as an argument to
	/// raw Lua API calls.
	inline operator lua_State *() { return L; }
	
	/////////////////////// Control over the Lua module system

	/// Sets the value of Lua's package.path variable.
	void SetPackagePath(const char *where);
	/// Install a library into the Lua state, and insert it into the _G table.
	void InstallGlobalLibrary(char const * name, lua_CFunction loader);
	/// Install a preloader into the package.preload table of this state.
	void InstallPreloader(char const * name, lua_CFunction loader);
	/// Run a preloader previously installed with Installpreloader
	void RunPreloader(char const * name);
	/// Run Lua's require() with the given module name, pushing the result to the stack.
	/// Returns false, leaving nil on the stack, if the require() fails.
	bool Require(char const * name, bool leaveError = false);

	/// Set the value of a state flag in the Lua registry.
	void setFlag(char const * name, bool value);
	/// Gets the value of a named state flag from the Lua registry.
	bool getFlag(const char *name);

	/// Loads a chunk from a string, pushing the resultant function to the stack.
	bool loadString(const char * code, bool leaveError = false);
	/// Evaluates a string with pcall.
	bool eval(const char * code);
	/// Evaluates a string with pcall, saving the error to the given std::string.
	/// If no error, string is not modified.
	bool evalAndGetError(const char * code, std::string & errmsg);
	/// Runs pcall, return false on error and captures the error message.
	bool pcall(int nargs, int nresults, std::string & errmsg);

	/// Unmarshal a value from Lua at the given stack index.
	template <typename T>
	inline bool get(int idx, T & x) { return LuaMarshal<T>::get(L, idx, x); }
	/// Check that the value at the given index can be unmarshalled.
	/// Lua error if not, unmarshal if so
	template <typename T>
	inline void check(int idx, T & x) { return LuaMarshal<T>::check(L, idx, x); }
	/// Marshal and push a value to Lua.
	template <typename T>
	inline void push(T const & x) { return LuaMarshal<T>::push(L, x); }
	/// Pop n items.
	inline void pop(int n) { lua_pop(L, n); }

};

} } // namespace oigroup::Lua


#endif /* LUASTATE_HPP_ */
