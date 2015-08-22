!include <win32.mak>

!if ( "$(CONFIGURATION)" == "StaticRelease" ) || ( "$(CONFIGURATION)" == "StaticDebug" )
NEEDSDLL=0
DISPATCH_OPTIONS=/DUSE_DISPATCH_STATIC_LIB=1
!elseif ( "$(CONFIGURATION)" == "DLLRelease" ) || ( "$(CONFIGURATION)" == "DLLDebug" )
NEEDSDLL=1
DISPATCH_OPTIONS=
!endif

cflags = $(cflags) /nologo /D "_UNICODE" /D "UNICODE"
lflags = $(lflags) /MANIFEST /INCREMENTAL:NO /NOLOGO /OPT:ICF /libpath:..\libdispatch\bin\$(PLATFORM)\$(CONFIGURATION) /libpath:"..\libdispatch++\bin\$(PLATFORM)\$(CONFIGURATION)" 

OBJS=$(INTDIR)dispatch_test.obj

all: dispatch_windows.exe dispatch_plusplus.exe dispatch_readsync.exe dispatch_timer_set_time.exe dispatch_debug.exe dispatch_pingpong.exe dispatch_starfish.exe dispatch_cascade.exe dispatch_priority_with_targets.exe dispatch_priority.exe dispatch_sema.exe dispatch_after.exe dispatch_apply.exe dispatch_api.exe dispatch_group.exe dispatch_drift.exe
	@if $(NEEDSDLL) == 1 copy ..\libdispatch\bin\$(PLATFORM)\$(CONFIGURATION)\libdispatch.dll $(OUTDIR)
	@if $(NEEDSDLL) == 1 copy "..\libdispatch++\bin\$(PLATFORM)\$(CONFIGURATION)\libdispatch++.dll" $(OUTDIR)

{src\}.c.obj:
	@if not exist $(INTDIR)$(NULL) mkdir $(INTDIR)
	@if not exist $(OUTDIR)$(NULL) mkdir $(OUTDIR)
	$(cc) $(cdebug) $(cflags) $(cvarsdll) /I"$(INCLUDE)" /Fo$(INTDIR) /Fd$(OUTDIR) $(DISPATCH_OPTIONS) $<

{src\}.cpp.obj:
	@if not exist $(INTDIR)$(NULL) mkdir $(INTDIR)
	@if not exist $(OUTDIR)$(NULL) mkdir $(OUTDIR)
	$(cc) $(cdebug) $(cflags) $(cvarsdll) /EHsc /I"$(INCLUDE)" /Fo$(INTDIR) /Fd$(OUTDIR) $(DISPATCH_OPTIONS) $<

$(INTDIR)dispatch_priority_with_targets.obj:
	@if not exist $(INTDIR)$(NULL) mkdir $(INTDIR)
	@if not exist $(OUTDIR)$(NULL) mkdir $(OUTDIR)
	$(cc) $(cdebug) $(cflags) $(cvarsdll) /I"$(INCLUDE)" /Fo$(INTDIR)dispatch_priority_with_targets.obj /Fd$(OUTDIR)dispatch_priority_with_targets.pdb /DUSE_SET_TARGET_QUEUE=1 $(DISPATCH_OPTIONS) src\dispatch_priority.c

{src\shims\}.c.obj:
	@if not exist $(INTDIR)shims\$(NULL) mkdir $(INTDIR)shims\
	@if not exist $(OUTDIR)shims\$(NULL) mkdir $(OUTDIR)shims\
	$(cc) $(cdebug) $(cflags) $(cvarsdll) /I"$(INCLUDE)" /Fo$(INTDIR)shims\ /Fd$(OUTDIR)shims\ $<

{$(INTDIR)}.obj.exe:
	$(link) $(ldebug) $(lflags) /PDB:$(OUTDIR)$(<B).pdb /out:$(OUTDIR)$(<B).exe $** $(guilibsdll) winmm.lib libdispatch.lib "libdispatch++.lib"

dispatch_api.exe: $(INTDIR)dispatch_api.obj $(OBJS)
dispatch_apply.exe: $(INTDIR)dispatch_apply.obj $(OBJS)
dispatch_after.exe: $(INTDIR)dispatch_after.obj $(OBJS)
dispatch_drift.exe: $(INTDIR)dispatch_drift.obj $(OBJS)
dispatch_group.exe: $(INTDIR)dispatch_group.obj $(OBJS)
dispatch_sema.exe: $(INTDIR)dispatch_sema.obj $(OBJS)
dispatch_priority.exe: $(INTDIR)dispatch_priority.obj $(OBJS)
dispatch_priority_with_targets.exe: $(INTDIR)dispatch_priority_with_targets.obj $(OBJS)
dispatch_cascade.exe: $(INTDIR)dispatch_cascade.obj $(OBJS)
dispatch_starfish.exe: $(INTDIR)dispatch_starfish.obj $(OBJS)
dispatch_pingpong.exe: $(INTDIR)dispatch_pingpong.obj $(OBJS)
dispatch_debug.exe: $(INTDIR)dispatch_debug.obj $(OBJS)
dispatch_timer_set_time.exe: $(INTDIR)dispatch_timer_set_time.obj $(OBJS)
dispatch_readsync.exe: $(INTDIR)dispatch_readsync.obj $(OBJS)
dispatch_plusplus.exe: $(INTDIR)dispatch_plusplus.obj $(OBJS)

dispatch_windows.exe: $(INTDIR)dispatch_windows.obj $(OBJS)
	@if not exist $(INTDIR)$(NULL) mkdir $(INTDIR)
	@if not exist $(OUTDIR)$(NULL) mkdir $(OUTDIR)
	$(link) $(ldebug) $(guilflags) /PDB:$(OUTDIR)dispatch_windows.pdb /out:$(OUTDIR)dispatch_windows.exe $** $(guilibsdll) winmm.lib libdispatch.lib "libdispatch++.lib"

rebuild: clean all

clean:
	$(CLEANUP)
