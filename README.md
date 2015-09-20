# MQ2Lua

A plugin for MacroQuest 2 allowing the use of the Lua scripting language.

## Why?

#### Coders

MQ2's internal programming language is... well, "less than ideal" is a generous way of putting it. 
If you've ever tried to script for MQ2, then you already know what I mean. 
Lua is a clean, fast, simple language designed for scenarios exactly like MQ2 scripting. 
If you're a coder, then it's an easy sell.

#### MQ2 Users

If you're a user, then you're probably here because someone else made something with MQ2Lua that you want to use.
Great! Follow the Quick Installation below. Make sure you also install MQ2LuaScripts!

## Quick Installation

- Exit EQ and MQ2.
- Put MQ2Lua.dll in your MacroQuest2 folder (wherever MacroQuest2.exe is)
- Make a folder called "lua" in your MacroQuest2 folder
- Download a collection of supporting scripts (like MQ2LuaScripts!) into the "lua" folder you just made.
- Launch MQ2, then EQ.
- Have fun!

**NOTE:** The MQ2Lua.dll is a deliberately minimalistic binding between MQ2 and Lua. 
The guiding philosophy behind its design is that whatever can be done in Lua, should be done 
in Lua. In fact, if you don't have a corresponding set of Lua scripts installed 
alongside it, MQ2Lua.dll will do nothing at all!

If you happen to be looking for that corresponding set of Lua scripts, why, I just so happen
to publish one on GitHub: MQ2LuaScripts. All the user-facing functionality is in there, so if you
are an end user looking to take advantage of Lua macros, or a scripter looking to make them,
go to MQ2LuaScripts.

On the other hand, if you are C++ coder interested in hacking on the actual DLL, extending the API,
or just want to learn more about the system, this is the place for you. Read on!

## Compiling from Source

Get the source into your MQ tree under MQ2Lua/ (I used mkplugin), then build with Visual Studio.
Make sure you have taken all the usual steps for compiling MQ2 plugins. (Consult MQ2 docs.)
The project files in the repository are all in the VS2015 format.

**MQ2Lua uses C++11, so you will not be able to compile it with ancient versions of Visual Studio.**
I know VS2015 works. I can't vouch for any previous versions.

## Lua environment

MQ2Lua creates a self-contained Lua environment when the plugin is launched. For user security reasons, the
environment does not include any of Lua's builtin operating-system or I/O functions. Specifically, the following
Lua builtin libraries are available, and no others:
	
	string, table, package, coroutine, math, bit32

The environment is governed by the Lua module system. Upon initialization, the following module search paths are
automatically added by the C code: ($MQ2_DIR is the directory where MacroQuest2.exe is located)

	$MQ2_DIR/lua/?.lua
	$MQ2_DIR/lua/?/init.lua
	$MQ2_DIR/lua/lib/?.lua
	$MQ2_DIR/lua/lib/?/init.lua

After it initializes, MQ2Lua will automatically execute ```require("Core")```. This means that
*creating a Lua module named Core somewhere in the above search path is your main entry point of interface with
MQ2Lua.*

The MQ2Lua API is exposed as a Lua module named "MQ2." To access this module in your code, use:

	local MQ2 = require("MQ2")

MQ2 will from time to time send events which may be of interest to you. There are two API functions
that will allow you to register Lua code to be invoked whenever these events happen: (See the API
docs for more about these functions)

	MQ2.pulse(pulseHandler)
	MQ2.events({eventHandlerTable})

MQ2Lua registers a chat command, "/lua", that allows the end user to interact with the Lua subsystem:

* ```/lua reload``` will destroy the current Lua state, unloading all code and freeing all memory. It
will then reload the Core.
* ```/lua (command) (args)``` will invoke ```events.command(command, args)``` (see MQ2.events below)
allowing users to interact with the running Lua modules.

And that's it! Everything else, you can do with Lua.

## Lua API

### MQ2.exec(string command)

Executes the given macro command. The command is executed at MQ2's global level (as if you had typed
it into a chat window) and MQ2 variable interpolation is enabled.

Example: ```MQ2.exec("/echo Hello world")``` => "Hello world" is printed to your MQ2 console.

Example: ```MQ2.exec("/casting \"Shield of Dreams\"")``` => Your character will cast Shield of Dreams (if you have the MQ2Cast plugin)

### value = MQ2.data(string dataVarName)

Retrieves the value of an MQ2 DataVar. Indices and members are processed, but MQ2 variables ARE NOT
interpolated. The value is converted into a plain Lua type in the most reasonable possible way.
Floats and ints are converted to Lua numbers. Strings and booleans are converted to Lua strings
and booleans.

If the DataVar would resolve to an MQ2 object, then it is cast to boolean, so "true" will be
returned if the object exists, "false" otherwise. If the DataVar can't be parsed or doesn't exist,
the Lua literal nil will be returned.

Example: ```MQ2.data("Me.PctHPs")``` => ```100```

Example: ```MQ2.data("Me.XTarget[1].TargetType")``` => ```"Auto Hater"```

Example: ```MQ2.data("Me")``` => ```true``` (Me is an object)

Example: ```MQ2.data("gibberish")``` => ```nil```

### MQ2.pulse(function pulseHandler)

Sets the pulse handler. This function will be pcall()ed every pulse. 

*WARNING:* Setting the pulse handler replaces the existing pulse handler! If you need multiple
pulse handlers, implement that in Lua. (See MQ2LuaScripts, which implements this for you!)

### MQ2.events(table eventHandlers)

Sets the table of callbacks for MQ2 events. When MQ2 notifies MQ2Lua of an event, MQ2Lua will
look in this table for the corresponding key. If the corresponding value is a function,
MQ2Lua will pcall() it. The following events are currently recognized:

* ```command(cmd, args)``` -- the user issued the MQ2 chat command ```/lua (cmd) (args)```
* ```zoned()``` -- MQ2 thinks the EverQuest client zoned. (WARNING: not always accurate)
* ```leftZone(), enteredZone()``` -- as zoned(), but for each half of the transition. (WARNING: not always accurate)
* ```shutdown()``` -- MQ2Lua is about to reload or exit; you should gracefully shutdown.
* ```leftWorld(), enteredWorld()``` -- The user moves between character select and in-game. 
```enteredWorld()``` is also called after a ```/lua reload``` provided the user is in game.
* ```gameStateChanged()``` -- Called when MQ2 calls ```SetGameState```. Use ```MQ2.gamestate()``` to get the gamestate.

*WARNING:* Setting an event handler table will clear out the existing one! If you need fancy
event handling, implement it in Lua. (See MQ2LuaScripts, which implements this for you!)

### number time = MQ2.clock()

A timer function. Result is a floating point number with units of seconds and precision of milliseconds.

### function f = MQ2.load(string filename)

Loads a lua file from within the $MQ2_PATH/lua/ directory. Returns a function which, when called, evaluates
the compiled code.

### MQ2.saveconfig(string filename, string data)

Writes a file named ```$(filename).config.lua``` to the $MQ2_PATH/lua/ directory. Its contents are given by
the literal data string. *NOTE:* if you wish to serialize a Lua object, you must encode it to a
string yourself! (Whatever can be done in Lua, should be!)

Combined with ```MQ2.load``` this can be used to develop a system for loading and storing user
configuration information.

### string state = MQ2.gamestate()

Retrieves a string describing the MQ2 gamestate. Possible values:

	INGAME, CHARCREATE, CHARSELECT, LOGGINGIN, PRECHARSELECT, UNLOADING, UNKNOWN

## What??? That's the whole API?

Yup! And you'd be surprised what you can do with that little. Check out MQ2LuaScripts.
