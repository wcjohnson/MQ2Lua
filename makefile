!include "../global.mak"

ALL : "$(OUTDIR)\MQ2Lua.dll"

CLEAN :
	-@erase "$(INTDIR)\MQ2Lua.obj"
	-@erase "$(INTDIR)\vc60.idb"
	-@erase "$(OUTDIR)\MQ2Lua.dll"
	-@erase "$(OUTDIR)\MQ2Lua.exp"
	-@erase "$(OUTDIR)\MQ2Lua.lib"
	-@erase "$(OUTDIR)\MQ2Lua.pdb"


LINK32=link.exe
LINK32_FLAGS=kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib $(DETLIB) ..\Release\MQ2Main.lib /nologo /dll /incremental:no /pdb:"$(OUTDIR)\MQ2Lua.pdb" /debug /machine:I386 /out:"$(OUTDIR)\MQ2Lua.dll" /implib:"$(OUTDIR)\MQ2Lua.lib" /OPT:NOICF /OPT:NOREF 
LINK32_OBJS= \
	"$(INTDIR)\MQ2Lua.obj" \
	"$(OUTDIR)\MQ2Main.lib"

"$(OUTDIR)\MQ2Lua.dll" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) $(LINK32_FLAGS) $(LINK32_OBJS)


!IF "$(NO_EXTERNAL_DEPS)" != "1"
!IF EXISTS("MQ2Lua.dep")
!INCLUDE "MQ2Lua.dep"
!ELSE 
!MESSAGE Warning: cannot find "MQ2Lua.dep"
!ENDIF 
!ENDIF 


SOURCE=.\MQ2Lua.cpp

"$(INTDIR)\MQ2Lua.obj" : $(SOURCE) "$(INTDIR)"

