//
// MQ2Lua.cpp
// (C)2015 Bill Johnson
// Lua scripting API for MacroQuest 2.
//

#include "../MQ2Plugin.h"
#include <oigroup/Lua/LuaState.hpp>
#include <oigroup/Lua/LuaMarshal.hpp>
#include <oigroup/Lua/LuaReferences.hpp>

#include <sstream>
#include <fstream>

using namespace oigroup::Lua;

PreSetup("MQ2Lua");

///////////////////////////// Lua state
LuaState * LS; // Global lua state.
bool isInWorld;
bool isZoning;
const char * gameState;
// Event handlers.
FunctionReference pulseHandler;

void printLuaError(const std::string & msg) {
	std::string s = "[Lua error] " + msg;
	WriteChatColor((PCHAR)s.c_str(), CONCOLOR_RED);
	//DebugSpewAlways((PCHAR)s.c_str());
}

void runScript(const char * code) {
	if (LS) {
		std::string luaError;
		if (!LS->evalAndGetError(code, luaError)) {
			printLuaError(luaError);
		}
	}
}

void didEnterZone() {
	isZoning = false;
	runScript("require(\"Core\")._enteredZone()");
}

void didLeaveZone() {
	isZoning = true;
	runScript("require(\"Core\")._leftZone()");
}

void didEnterWorld() {
	isInWorld = true;
	runScript("require(\"Core\")._enteredWorld()");
	didEnterZone();
}

void didLeaveWorld() {
	didLeaveZone();
	isInWorld = false;
	runScript("require(\"Core\")._leftWorld()");
}

void initLuaState() {
	if (!LS) {
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
	} else {
		DebugSpewAlways("Lua alread initialized");
	}
}

void teardownLuaState() {
	if (LS) {
		//DebugSpewAlways("Shutting down Lua.");
		// Ask state to shutdown as gracefully as possible
		runScript("require(\"Core\")._shutdown()");
		// Destroy any references we might be holding to stuff inside this state
		pulseHandler.Free();
		// Destroy the state.
		delete LS; LS = nullptr; 
		//DebugSpewAlways("Lua shutdown successful.");
	} else {
		DebugSpewAlways("Lua already shut down");
	}
	
}

void reloadLua() {
	teardownLuaState();
	initLuaState();
}


/////////////////////////////////// Lua API to call MQ2.
static int MQ2_log(lua_State * L) {
	std::stringstream logline;
	// Describe where it happened in the Lua source code if possible
	lua_Debug d;
	if (lua_getstack(L, 1, &d)) {
		if (lua_getinfo(L, "Sl", &d)) {
			logline << " <Lua: " << d.short_src << "@" << d.currentline << "> ";
		}
	}
	// Concatenate the args.
	int n = lua_gettop(L);
	std::string part;
	for (int i = 1; i <= n; ++i) {
		LuaCheck(L, i, part);
		logline << part;
	}
	// Print to chat
	std::string s = logline.str();
	const char *unfunf = s.c_str();
	WriteChatColor((PCHAR)unfunf, CONCOLOR_YELLOW);
	//DebugSpewAlways((PCHAR)unfunf);
	return 0;
}

static int MQ2_print(lua_State * L) {
	std::stringstream logline;
	logline << "[MQ2Lua] ";
	// Concatenate the args.
	int n = lua_gettop(L);
	std::string part;
	for (int i = 1; i <= n; ++i) {
		LuaCheck(L, i, part);
		logline << part;
	}
	// Print to chat
	WriteChatColor((PCHAR)logline.str().c_str());
	return 0;
}

static int MQ2_exec(lua_State * L) {
	std::string cmd;
	LuaCheck(L, 1, cmd);
	EzCommand((PCHAR)cmd.c_str());
	return 0;
}

static int MQ2_data(lua_State * L) {
	const char * cmd;
	char cmdBuf[MAX_STRING];
	MQ2TYPEVAR rst;
	// Demarshal the datavar name.
	LuaCheck(L, 1, cmd);
	// Soo... ParseMQ2DataPortion mutates the passed string and therefore fucks up the Lua state
	// unless we copy it to a separate buffer. Madness.
	strncpy(cmdBuf, cmd, MAX_STRING); cmdBuf[MAX_STRING - 1] = '\0';
	if (!ParseMQ2DataPortion(cmdBuf, rst)) {
		lua_pushnil(L);
		return 1;
	} else {
		if (rst.Type == pBoolType) {
			if (rst.DWord == 0) LuaPush(L, false); else LuaPush(L, true);
			return 1;
		}
		else if (rst.Type == pFloatType) {
			LuaPush(L, rst.Float);
			return 1;
		}
		else if (rst.Type == pDoubleType) {
			LuaPush(L, rst.Double);
			return 1;
		}
		else if (rst.Type == pIntType) {
			LuaPush(L, rst.Int);
			return 1;
		}
		else if (rst.Type == pInt64Type) {
			LuaPush(L, (lua_Number)rst.Int64);
			return 1;
		}
		else if (rst.Type == pStringType) {
			LuaPush(L, (const char *)rst.Ptr);
			return 1;
		}
		else if (rst.Type == pByteType) {
			char x[2];
			x[0] = (char)(rst.DWord % 0xFF); x[1] = (char)0;
			LuaPush(L, (const char *)(&x[0]));
			return 1;
		}
		else {
			//return luaL_argerror(L, 1, "DataVar must resolve to a pure type, not an object.");
			// Cast objects to BOOL.
			if (rst.DWord) LuaPush(L, true); else LuaPush(L, false);
			return 1;
		}
	}
}

static int MQ2_event(lua_State * L) {
	std::string eventName;
	LuaCheck(L, 1, eventName);
	// Demarshal a Lua function to call when the corresponding event happens.
	// XXX: this is not very efficient, but this function is rarely called anyway.
	if (eventName == "pulse") {
		LuaCheck(L, 2, pulseHandler);
		return 0;
	} else {
		return luaL_argerror(L, 1, "invalid event name");
	}
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
	// XXX: make sure no path separators or dots
	std::string fileName;
	LuaCheck(L, 1, fileName);
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
		EXPORT_TO_LUA(MQ2_log, log);
		EXPORT_TO_LUA(MQ2_exec, exec);
		EXPORT_TO_LUA(MQ2_data, data);
		EXPORT_TO_LUA(MQ2_event, event);
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
	runScript(cmd);
}

void CmdLuaReload(PSPAWNINFO pChar, char* cmd) {
	if (LS) {
		reloadLua();
	}
}

/////////////////////////////////////////////////////// Plugin DLL entry points

// Called once, when the plugin is to initialize
PLUGIN_API VOID InitializePlugin(VOID)
{
    DebugSpewAlways("Initializing MQ2Lua");
	isInWorld = false; isZoning = false; gameState = "UNKNOWN";
	initLuaState();
    //Add commands, MQ2Data items, hooks, etc.
	AddCommand("/lua", CmdLua);
	AddCommand("/luareload", CmdLuaReload);
    //AddCommand("/mycommand",MyCommand);
    //AddXMLFile("MQUI_MyXMLFile.xml");
    //bmMyBenchmark=AddMQ2Benchmark("My Benchmark Name");
}

// Called once, when the plugin is to shutdown
PLUGIN_API VOID ShutdownPlugin(VOID)
{
    DebugSpewAlways("Shutting down MQ2Lua");
	RemoveCommand("/luareload");
	RemoveCommand("/lua");
	teardownLuaState();

    //Remove commands, MQ2Data items, hooks, etc.
    //RemoveMQ2Benchmark(bmMyBenchmark);
    //RemoveCommand("/mycommand");
    //RemoveXMLFile("MQUI_MyXMLFile.xml");
}

// Called after entering a new zone
PLUGIN_API VOID OnZoned(VOID)
{
	WriteChatColor("MQ2Lua::OnZoned()");
	runScript("require(\"Core\")._zoned()");
}

// Called once directly before shutdown of the new ui system, and also
// every time the game calls CDisplay::CleanGameUI()
PLUGIN_API VOID OnCleanUI(VOID)
{
    WriteChatColor("MQ2Lua::OnCleanUI()");
    // destroy custom windows, etc
}

// Called once directly after the game ui is reloaded, after issuing /loadskin
PLUGIN_API VOID OnReloadUI(VOID)
{
    WriteChatColor("MQ2Lua::OnReloadUI()");
    // recreate custom windows, etc
}

// Called every frame that the "HUD" is drawn -- e.g. net status / packet loss bar
PLUGIN_API VOID OnDrawHUD(VOID)
{
    // DONT leave in this debugspew, even if you leave in all the others
    //DebugSpewAlways("MQ2Lua::OnDrawHUD()");
}

// Called once directly after initialization, and then every time the gamestate changes
PLUGIN_API VOID SetGameState(DWORD GameState)
{
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
	runScript("require(\"Core\")._gameStateChanged()");

	// Fire didLeaveWorld events.
	switch (GameState) {
	case GAMESTATE_INGAME:
		if (!isInWorld) didEnterWorld();
		if (isZoning) didEnterZone();
		break;
	case GAMESTATE_LOGGINGIN:
		// MQ2 fires this when we zone.
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
PLUGIN_API VOID OnPulse(VOID)
{
    // DONT leave in this debugspew, even if you leave in all the others
    //DebugSpewAlways("MQ2Lua::OnPulse()");

	if (LS && pulseHandler.IsValid()) {
		if (pulseHandler.Push(*LS)) {
			if (lua_pcall(*LS, 0, 0, 0)) {
				std::string luaError;
				LS->get(1, luaError); LS->pop(1);
				printLuaError(luaError);
			}
		} else {
			LS->pop(1);
		}
	}
}

// This is called every time WriteChatColor is called by MQ2Main or any plugin,
// IGNORING FILTERS, IF YOU NEED THEM MAKE SURE TO IMPLEMENT THEM. IF YOU DONT
// CALL CEverQuest::dsp_chat MAKE SURE TO IMPLEMENT EVENTS HERE (for chat plugins)
PLUGIN_API DWORD OnWriteChatColor(PCHAR Line, DWORD Color, DWORD Filter)
{
    //DebugSpewAlways("MQ2Lua::OnWriteChatColor(%s)",Line);
    return 0;
}

// This is called every time EQ shows a line of chat with CEverQuest::dsp_chat,
// but after MQ filters and chat events are taken care of.
PLUGIN_API DWORD OnIncomingChat(PCHAR Line, DWORD Color)
{
    //DebugSpewAlways("MQ2Lua::OnIncomingChat(%s)",Line);
    return 0;
}

// This is called each time a spawn is added to a zone (inserted into EQ's list of spawns),
// or for each existing spawn when a plugin first initializes
// NOTE: When you zone, these will come BEFORE OnZoned
PLUGIN_API VOID OnAddSpawn(PSPAWNINFO pNewSpawn)
{
    //DebugSpewAlways("MQ2Lua::OnAddSpawn(%s)",pNewSpawn->Name);
}

// This is called each time a spawn is removed from a zone (removed from EQ's list of spawns).
// It is NOT called for each existing spawn when a plugin shuts down.
PLUGIN_API VOID OnRemoveSpawn(PSPAWNINFO pSpawn)
{
    //DebugSpewAlways("MQ2Lua::OnRemoveSpawn(%s)",pSpawn->Name);
	//pSpawn->SpawnID
}

// This is called each time a ground item is added to a zone
// or for each existing ground item when a plugin first initializes
// NOTE: When you zone, these will come BEFORE OnZoned
PLUGIN_API VOID OnAddGroundItem(PGROUNDITEM pNewGroundItem)
{
    //DebugSpewAlways("MQ2Lua::OnAddGroundItem(%d)",pNewGroundItem->DropID);
}

// This is called each time a ground item is removed from a zone
// It is NOT called for each existing ground item when a plugin shuts down.
PLUGIN_API VOID OnRemoveGroundItem(PGROUNDITEM pGroundItem)
{
    //DebugSpewAlways("MQ2Lua::OnRemoveGroundItem(%d)",pGroundItem->DropID);
}
