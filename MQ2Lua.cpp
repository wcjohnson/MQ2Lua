//
// MQ2Lua.cpp
// (C)2015 Bill Johnson
// Lua scripting API for MacroQuest 2.
//

#include "../MQ2Plugin.h"
#include <oigroup/Lua/LuaState.hpp>
#include <oigroup/Lua/LuaMarshal.hpp>
#include <oigroup/Lua/LuaReferences.hpp>
#include <oigroup/Lua/LuaStackMarker.hpp>

#include <sstream>
#include <fstream>

using namespace oigroup::Lua;

PreSetup("MQ2Lua");

///////////////////////////// Lua state
LuaState * LS; // Global lua state.
bool isInWorld;
bool isZoning;
bool shouldReloadOnNextPulse; // Defer reload for a pulse so Lua can ask Lua to reload without blowing up Lua.
const char * gameState;
// Event handlers.
FunctionReference pulseHandler;
TableReference eventsHandler;

void printLuaError(const std::string & msg) {
	std::string s = "[Lua error] " + msg;
	WriteChatColor((PCHAR)s.c_str(), CONCOLOR_RED);
	//DebugSpewAlways((PCHAR)s.c_str());
}

void runScript(const char * code) {
	if (!LS) return;
	std::string luaError;
	if (!LS->evalAndGetError(code, luaError)) {
		printLuaError(luaError);
	}
}

template <typename... Args>
void callEventHandler(const char * eventName, Args const &... args) {
	constexpr int nargs = sizeof...(Args);
	// Get the event function.
	if ( (!LS) || (!eventName) || (!eventsHandler.IsValid()) ) return;
	// Evaluate eventsHandler[eventName]
	eventsHandler.Push(*LS);
	LS->push(eventName);
	lua_gettable(*LS, -2);
	lua_remove(*LS, -2);
	// eventsHandler[eventName] is the only thing on the stack now.
	// Make sure it's a function
	if (lua_type(*LS, -1) != LUA_TFUNCTION) {
		lua_pop(*LS, 1); return;
	}
	// Push all args for the event handler
	LuaVariadicPush(*LS, args...);
	// Stack is now ready for a pcall
	std::string errmsg;
	if (!LS->pcall(nargs, 0, errmsg)) { printLuaError(errmsg); }
}

void didEnterZone() {
	isZoning = false; callEventHandler("enteredZone");
}

void didLeaveZone() {
	isZoning = true; callEventHandler("leftZone");
}

void didEnterWorld() {
	isInWorld = true; callEventHandler("enteredWorld");
	// Entering world counts as entering zone, I guess.
	didEnterZone();
}

void didLeaveWorld() {
	didLeaveZone(); 
	isInWorld = false; callEventHandler("leftWorld");
}

void initLuaState() {
	if (LS) return; // lua already initialized

	LS = new LuaState();
	// Install exotic libraries.
	LS->InstallGlobalLibrary("coroutine", luaopen_coroutine);
	// Build lua macros path
	// gszIniPath is where the ini is.
	std::string luaPath(gszINIPath);
	std::string luaModuleString = luaPath + "/lua/?.lua;" + luaPath + "/lua/?/init.lua;" + luaPath + "/lua/lib/?.lua;" + luaPath + "/lua/lib/?/init.lua";
	LS->SetPackagePath(luaModuleString.c_str());
	//DebugSpewAlways("Initialized Lua with module path %s", luaModuleString.c_str());
	// Load the core module.
	runScript("_G.Core = require(\"Core\")");
	// If we were already in the world, this was a /reload.
	// Re-invoke didEnterWorld.
	if (isInWorld) didEnterWorld();
}

void teardownLuaState() {
	if (!LS) return;

	callEventHandler("shutdown");
	// Destroy any references we might be holding to stuff inside this state
	pulseHandler.Free();
	eventsHandler.Free();
	// Destroy the state.
	delete LS; LS = nullptr;	
}

void reloadLua() {
	teardownLuaState(); initLuaState();
}


/////////////////////////////////// Lua API to call MQ2.

// Push an mq2 data object to the Lua stack.
int pushMQ2Data(lua_State * L, MQ2TYPEVAR & rst) {
	if (rst.Type == pBoolType) {
		if (rst.DWord == 0) LuaPush(L, false); else LuaPush(L, true);
		return 1;
	} else if (rst.Type == pFloatType) {
		LuaPush(L, rst.Float);
		return 1;
	} else if (rst.Type == pDoubleType) {
		LuaPush(L, rst.Double);
		return 1;
	} else if (rst.Type == pIntType) {
		LuaPush(L, rst.Int);
		return 1;
	} else if (rst.Type == pInt64Type) {
		LuaPush(L, (lua_Number)rst.Int64);
		return 1;
	} else if (rst.Type == pStringType) {
		LuaPush(L, (const char *)rst.Ptr);
		return 1;
	} else if (rst.Type == pByteType) {
		char x[2];
		x[0] = (char)(rst.DWord % 0xFF); x[1] = (char)0;
		LuaPush(L, (const char *)(&x[0]));
		return 1;
	} else {
		// Cast objects to BOOL.
		if (rst.DWord) LuaPush(L, true); else LuaPush(L, false);
		return 1;
	}
}

// Print string to mq2 chat window.
static int MQ2_print(lua_State * L) {
	std::stringstream logline;
	logline << "[MQ2Lua] ";
	// Concatenate the args.
	int n = lua_gettop(L);
	std::string part;
	for (int i = 1; i <= n; ++i) {
		LuaCheck(L, i, part); logline << part;
	}
	// Print to chat
	WriteChatColor((PCHAR)logline.str().c_str());
	return 0;
}

// Run a slash command.
static int MQ2_exec(lua_State * L) {
	std::string cmd;
	LuaCheck(L, 1, cmd);
	EzCommand((PCHAR)cmd.c_str());
	return 0;
}

// Access an MQ2 datavar.
static int MQ2_data(lua_State * L) {
	const char * cmd;
	char cmdBuf[MAX_STRING];
	MQ2TYPEVAR rst;
	// Demarshal the datavar name.
	LuaCheck(L, 1, cmd);
	// XXX: Soo... ParseMQ2DataPortion MUTATES the passed string (WHYYYYYYYYYYYYYYY)
	// and therefore fucks up the Lua state unless we dup it to a separate buffer.
	strncpy(cmdBuf, cmd, MAX_STRING); cmdBuf[MAX_STRING - 1] = '\0';
	if (!ParseMQ2DataPortion(cmdBuf, rst)) {
		lua_pushnil(L);
		return 1;
	} else {
		return pushMQ2Data(L, rst);
	}
}


bool Lgetstr(lua_State * L, int idx, char * buf, size_t max) {
	// Clear buffer
	buf[0] = '\0';
	if (!lua_isstring(L, idx)) return false;
	const char *str = lua_tolstring(L, idx, nullptr);
	if (!str) return false;
	strncpy(buf, str, max); buf[max - 1] = '\0';
	return true;
}

// Access MQ2 datavars without going through the parser.
static int MQ2_xdata(lua_State * L) {
	// xdata(a1, a2, a3, a4, a5, a6, ...) is like data("a1[a2].a3[a4].a5[a6]...")
	int n = lua_gettop(L);
	MQ2TYPEVAR accum;
	// char member[MAX_STRING]; char index[MAX_STRING]; // Buffered version
	const char * member = ""; const char * index = ""; // Direct version

	////////////// First get the TLO, which is arg1.
	//if (!Lgetstr(L, 1, member, MAX_STRING)) { lua_pushnil(L); return 1; }
	if (!LuaGet(L, 1, member)) { lua_pushnil(L); return 1; }

	PMQ2DATAITEM tlo = FindMQ2Data((char*)member);
	if (!tlo) { lua_pushnil(L); return 1; } // invalid TLO
	
	////////////// Now get the TLO's index if any.
	//if(n >= 2) Lgetstr(L, 2, index, MAX_STRING);
	if (n >= 2) LuaGet(L, 2, index);

	// Evaluate the TLO at the index.
	if (!tlo->Function((char*)index, accum)) { lua_pushnil(L); return 1; } 

	////////////// Now continue evaluating members and indices.
	// Iterate further member/index pairs.
	for (int i = 3; i <= n; i = i + 2) {
		if (!accum.Type) { lua_pushnil(L); return 1; } // Somehow we got a non-object; abort.

		// Reset parse vars
		member = ""; index = "";

		// If no member, abort.
		//if (!Lgetstr(L, i, member, MAX_STRING)) break;
		if (!LuaGet(L, i, member)) break;

		// Grab index if present
		//if (n >= i + 1) Lgetstr(L, i + 1, index, MAX_STRING); else index[0] = '\0';
		if (n >= i + 1) LuaGet(L, i + 1, index);
		
		// Execute GetMember, putting the result back into the accumulator for further indexing.
		if (!accum.Type->GetMember(accum.VarPtr, (char*)member, (char*)index, accum)) {
			lua_pushnil(L); return 1;
		}
	}

	/////////////////////// Return the final object after indexing and member processing.
	return pushMQ2Data(L, accum);
}


// Registers a table of event handlers to be called back on MQ2 events.
static int MQ2_events(lua_State * L) {
	LuaCheck(L, 1, eventsHandler); return 0;
}
static int MQ2_pulse(lua_State * L) {
	LuaCheck(L, 1, pulseHandler); return 0;
}

static int MQ2_clock(lua_State *L) {
	lua_pushnumber(L, ((lua_Number)clock()) / (lua_Number)CLOCKS_PER_SEC);
	return 1;
}

static int MQ2_load(lua_State *L) {
	// Get filename
	std::string fileName;
	LuaCheck(L, 1, fileName);
	// XXX: make sure no path separators
	if (fileName.find("..") != std::string::npos) {
		return luaL_argerror(L, 1, "double-dot (..) is forbidden in filenames");
	}
	std::string basepath(gszINIPath);
	fileName = basepath + "/lua/" + fileName;
	// Load file
	if (luaL_loadfile(L, fileName.c_str())) {
		return lua_error(L);
	} else {
		return 1;
	}
}

static int MQ2_saveconfig(lua_State *L) {
	// Get filename
	std::string fileName;
	LuaCheck(L, 1, fileName);
	// XXX: make sure no path separators
	if (fileName.find("..") != std::string::npos) {
		return luaL_argerror(L, 1, "double-dot (..) is forbidden in filenames");
	}
	std::string basepath(gszINIPath);
	fileName = basepath + "/lua/" + fileName + ".config.lua";
	// Get data to save
	std::string data;
	LuaCheck(L, 2, data);
	// Save it
	std::ofstream out(fileName, std::ofstream::trunc);
	if (out.good()) {
		out << data;
		out.close();
		if (!out.fail()) {
			return 0;
		}
	}
	return luaL_error(L, "couldn't save file");
}

static int MQ2_gamestate(lua_State * L) {
	LuaPush(L, gameState);
	return 1;
}

#define EXPORT_TO_LUA(func, name) lua_pushcfunction(L, (func)); lua_setfield(L, -2, (#name));
struct lua_initializer {
	static int loader(lua_State * L) {
		lua_createtable(L, 0, 0);

		EXPORT_TO_LUA(MQ2_print, print);
		EXPORT_TO_LUA(MQ2_exec, exec);
		EXPORT_TO_LUA(MQ2_data, data);
		EXPORT_TO_LUA(MQ2_xdata, xdata);
		EXPORT_TO_LUA(MQ2_events, events);
		EXPORT_TO_LUA(MQ2_pulse, pulse);
		EXPORT_TO_LUA(MQ2_clock, clock);
		EXPORT_TO_LUA(MQ2_load, load);
		EXPORT_TO_LUA(MQ2_saveconfig, saveconfig);
		EXPORT_TO_LUA(MQ2_gamestate, gamestate);

		return 1;
	}
	static void init(LuaState & LS) {
		if (LS.getFlag("MQ2.Preloaded")) return;
		LS.InstallPreloader("MQ2", loader);
		LS.setFlag("MQ2.Preloaded", true);
	}
	lua_initializer() { LuaState::RegisterGlobalInitFunc(init); }
} _init_mq2lua;

////////////////////////////////////////////////////// Slash commands

void CmdLua(PSPAWNINFO pChar, char* cmd) {
	if (!LS) return;
	// Parse the command
	std::istringstream ss(cmd);
	std::string command;
	std::string rest("");
	if ( (!std::getline(ss, command, ' ')) || command.length() == 0) {
		printLuaError("Empty command");
		return;
	}
	// Special case: reload
	if (command == "reload") {
		shouldReloadOnNextPulse = true;
		return;
	}
	// Load rest of args
	std::getline(ss, rest);
	// Exec lua event handler
	callEventHandler("command", command, rest);
}


/////////////////////////////////////////////////////// Plugin DLL entry points
// Called once, when the plugin is to initialize
PLUGIN_API VOID InitializePlugin(VOID) {
	shouldReloadOnNextPulse = false;
	isInWorld = false; isZoning = false; gameState = "UNKNOWN";
	initLuaState();
	AddCommand("/lua", CmdLua);
}

PLUGIN_API VOID ShutdownPlugin(VOID) {
	RemoveCommand("/lua");
	teardownLuaState();
}

// Called after entering a new zone
PLUGIN_API VOID OnZoned(VOID) {
	callEventHandler("zoned");
}

// Called once directly before shutdown of the new ui system, and also
// every time the game calls CDisplay::CleanGameUI()
PLUGIN_API VOID OnCleanUI(VOID) {
	callEventHandler("cleanUI");
    // destroy custom windows, etc
}

// Called once directly after the game ui is reloaded, after issuing /loadskin
PLUGIN_API VOID OnReloadUI(VOID) {
	callEventHandler("reloadUI");
    // recreate custom windows, etc
}

// Called every frame that the "HUD" is drawn -- e.g. net status / packet loss bar
PLUGIN_API VOID OnDrawHUD(VOID) {
	callEventHandler("drawHUD");
}

// Called once directly after initialization, and then every time the gamestate changes
PLUGIN_API VOID SetGameState(DWORD GameState) {
	// Fire GameStateChanged events.
	switch (GameState) {
	case GAMESTATE_INGAME:
		gameState = "INGAME"; break;
	case GAMESTATE_CHARCREATE:
		gameState = "CHARCREATE"; break;
	case GAMESTATE_CHARSELECT:
		gameState = "CHARSELECT"; break;
	case GAMESTATE_LOGGINGIN:
		gameState = "LOGGINGIN"; break;
	case GAMESTATE_PRECHARSELECT:
		gameState = "PRECHARSELECT"; break;
	case GAMESTATE_UNLOADING:
		gameState = "UNLOADING"; break;
	case GAMESTATE_SOMETHING:
	default:
		gameState = "UNKNOWN"; break;
	}
	callEventHandler("gameStateChanged");

	// Fire didLeaveWorld events.
	switch (GameState) {
	case GAMESTATE_INGAME:
		if (!isInWorld) didEnterWorld();
		if (isZoning) didEnterZone();
		break;
	case GAMESTATE_LOGGINGIN:
		// MQ2 sometimes fires this instead of DidZone() when we zone.
		if (!isZoning) didLeaveZone();
		break;
	case GAMESTATE_CHARCREATE:
	case GAMESTATE_CHARSELECT:
	case GAMESTATE_PRECHARSELECT:
	case GAMESTATE_UNLOADING:
	case GAMESTATE_SOMETHING:
	default:
		if (isInWorld) didLeaveWorld();
		break;
	}

    //if (GameState==GAMESTATE_INGAME)
    // create custom windows if theyre not set up, etc
}


// This is called every time MQ pulses
PLUGIN_API VOID OnPulse(VOID) {
	if (shouldReloadOnNextPulse) {
		shouldReloadOnNextPulse = false;
		reloadLua();
		return;
	}

	if (!LS) return;

	if (pulseHandler.Push(*LS)) {
		std::string luaError;
		if (!LS->pcall(0, 0, luaError)) {
			printLuaError(luaError);
		}
	} else {
		// pulseHandler.Push leaves nil on the stack when it fails
		LS->pop(1);
	}
}

// This is called every time WriteChatColor is called by MQ2Main or any plugin,
// IGNORING FILTERS, IF YOU NEED THEM MAKE SURE TO IMPLEMENT THEM. IF YOU DONT
// CALL CEverQuest::dsp_chat MAKE SURE TO IMPLEMENT EVENTS HERE (for chat plugins)
PLUGIN_API DWORD OnWriteChatColor(PCHAR Line, DWORD Color, DWORD Filter) {
	callEventHandler("onWriteChatColor");
    return 0;
}

// This is called every time EQ shows a line of chat with CEverQuest::dsp_chat,
// but after MQ filters and chat events are taken care of.
PLUGIN_API DWORD OnIncomingChat(PCHAR Line, DWORD Color) {
	callEventHandler("onIncomingChat");
    return 0;
}

// This is called each time a spawn is added to a zone (inserted into EQ's list of spawns),
// or for each existing spawn when a plugin first initializes
// NOTE: When you zone, these will come BEFORE OnZoned
PLUGIN_API VOID OnAddSpawn(PSPAWNINFO pNewSpawn) {
	callEventHandler("onAddSpawn", (lua_Number)(pNewSpawn->SpawnID) );
}

// This is called each time a spawn is removed from a zone (removed from EQ's list of spawns).
// It is NOT called for each existing spawn when a plugin shuts down.
PLUGIN_API VOID OnRemoveSpawn(PSPAWNINFO pSpawn) {
	callEventHandler("onRemoveSpawn", (lua_Number)(pSpawn->SpawnID));
}

// This is called each time a ground item is added to a zone
// or for each existing ground item when a plugin first initializes
// NOTE: When you zone, these will come BEFORE OnZoned
PLUGIN_API VOID OnAddGroundItem(PGROUNDITEM pNewGroundItem) {
	callEventHandler("onAddGroundItem", (lua_Number)(pNewGroundItem->DropID) );
}

// This is called each time a ground item is removed from a zone
// It is NOT called for each existing ground item when a plugin shuts down.
PLUGIN_API VOID OnRemoveGroundItem(PGROUNDITEM pGroundItem) {
	callEventHandler("onRemoveGroundItem", (lua_Number)(pGroundItem->DropID));
}
