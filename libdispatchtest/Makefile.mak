!include <win32.mak>

!if ( "$(CONFIGURATION)" == "StaticRelease" ) || ( "$(CONFIGURATION)" == "StaticDebug" )
NEEDSDLL=0
DISPATCH_OPTIONS=/DUSE_LIB=1
!elseif ( "$(CONFIGURATION)" == "DLLRelease" ) || ( "$(CONFIGURATION)" == "DLLDebug" )
NEEDSDLL=1
DISPATCH_OPTIONS=
!endif

cflags = $(cflags) /D "_UNICODE" /D "UNICODE"
lflags = $(lflags) /MANIFEST /INCREMENTAL:NO /NOLOGO /OPT:ICF 

all: dispatch_windows.exe dispatch_plusplus.exe dispatch_readsync.exe dispatch_timer_set_time.exe dispatch_debug.exe dispatch_pingpong.exe dispatch_starfish.exe dispatch_cascade.exe dispatch_priority_with_targets.exe dispatch_priority.exe dispatch_sema.exe dispatch_after.exe dispatch_apply.exe dispatch_api.exe dispatch_group.exe dispatch_drift.exe
	@if $(NEEDSDLL) == 1 copy ..\libdispatch\bin\$(PLATFORM)\$(CONFIGURATION)\libdispatch.dll $(OUTDIR)
	@if $(NEEDSDLL) == 1 copy "..\libdispatch++\bin\$(PLATFORM)\$(CONFIGURATION)\libdispatch++.dll" $(OUTDIR)

{src\}.c.obj:
	if not exist $(INTDIR)$(NULL) mkdir $(INTDIR)
	if not exist $(OUTDIR)$(NULL) mkdir $(OUTDIR)
	$(cc) $(cdebug) $(cflags) $(cvarsdll) -I"$(INCLUDE)" /Fo$(INTDIR) /Fd$(OUTDIR) $(DISPATCH_OPTIONS) $<

{src\}.cpp.obj:
	if not exist $(INTDIR)$(NULL) mkdir $(INTDIR)
	if not exist $(OUTDIR)$(NULL) mkdir $(OUTDIR)
	$(cc) $(cdebug) $(cflags) $(cvarsdll) -EHsc -I"$(INCLUDE)" /Fo$(INTDIR) /Fd$(OUTDIR) $(DISPATCH_OPTIONS) $<

$(INTDIR)dispatch_priority_with_targets.obj:
	if not exist $(INTDIR)$(NULL) mkdir $(INTDIR)
	if not exist $(OUTDIR)$(NULL) mkdir $(OUTDIR)
	$(cc) $(cdebug) $(cflags) $(cvarsdll) -I"$(INCLUDE)" /Fo$(INTDIR)dispatch_priority_with_targets.obj /Fd$(OUTDIR)dispatch_priority_with_targets.pdb /DUSE_SET_TARGET_QUEUE=1 $(DISPATCH_OPTIONS) src\dispatch_priority.c

{src\shims\}.c.obj:
	if not exist $(INTDIR)shims\$(NULL) mkdir $(INTDIR)shims\
	if not exist $(OUTDIR)shims\$(NULL) mkdir $(OUTDIR)shims\
	$(cc) $(cdebug) $(cflags) $(cvarsdll) -I"$(INCLUDE)" /Fo$(INTDIR)shims\ /Fd$(OUTDIR)shims\ $<

dispatch_api.exe: $(INTDIR)dispatch_api.obj $(INTDIR)dispatch_test.obj
	$(link) $(ldebug) $(lflags) /libpath:..\libdispatch\bin\$(PLATFORM)\$(CONFIGURATION) -out:$(OUTDIR)dispatch_api.exe $** $(guilibsdll) libdispatch.lib

dispatch_apply.exe: $(INTDIR)dispatch_apply.obj $(INTDIR)dispatch_test.obj
	$(link) $(ldebug) $(lflags) /libpath:..\libdispatch\bin\$(PLATFORM)\$(CONFIGURATION) -out:$(OUTDIR)dispatch_apply.exe $** $(guilibsdll) libdispatch.lib

dispatch_after.exe: $(INTDIR)dispatch_after.obj $(INTDIR)dispatch_test.obj
	$(link) $(ldebug) $(lflags) /libpath:..\libdispatch\bin\$(PLATFORM)\$(CONFIGURATION) -out:$(OUTDIR)dispatch_after.exe $** $(guilibsdll) libdispatch.lib

dispatch_drift.exe: $(INTDIR)dispatch_drift.obj $(INTDIR)dispatch_test.obj
	$(link) $(ldebug) $(lflags) /libpath:..\libdispatch\bin\$(PLATFORM)\$(CONFIGURATION) -out:$(OUTDIR)dispatch_drift.exe $** $(guilibsdll) libdispatch.lib winmm.lib

dispatch_group.exe: $(INTDIR)dispatch_group.obj $(INTDIR)dispatch_test.obj
	$(link) $(ldebug) $(lflags) /libpath:..\libdispatch\bin\$(PLATFORM)\$(CONFIGURATION) -out:$(OUTDIR)dispatch_group.exe $** $(guilibsdll) libdispatch.lib

dispatch_sema.exe: $(INTDIR)dispatch_sema.obj $(INTDIR)dispatch_test.obj
	$(link) $(ldebug) $(lflags) /libpath:..\libdispatch\bin\$(PLATFORM)\$(CONFIGURATION) -out:$(OUTDIR)dispatch_sema.exe $** $(guilibsdll) libdispatch.lib

dispatch_priority.exe: $(INTDIR)dispatch_priority.obj $(INTDIR)dispatch_test.obj
	$(link) $(ldebug) $(lflags) /libpath:..\libdispatch\bin\$(PLATFORM)\$(CONFIGURATION) -out:$(OUTDIR)dispatch_priority.exe $** $(guilibsdll) libdispatch.lib

dispatch_priority_with_targets.exe: $(INTDIR)dispatch_priority_with_targets.obj $(INTDIR)dispatch_test.obj
	$(link) $(ldebug) $(lflags) /libpath:..\libdispatch\bin\$(PLATFORM)\$(CONFIGURATION) -out:$(OUTDIR)dispatch_priority_with_targets.exe $** $(guilibsdll) libdispatch.lib

dispatch_cascade.exe: $(INTDIR)dispatch_cascade.obj $(INTDIR)dispatch_test.obj
	$(link) $(ldebug) $(lflags) /libpath:..\libdispatch\bin\$(PLATFORM)\$(CONFIGURATION) -out:$(OUTDIR)dispatch_cascade.exe $** $(guilibsdll) libdispatch.lib

dispatch_starfish.exe: $(INTDIR)dispatch_starfish.obj $(INTDIR)dispatch_test.obj
	$(link) $(ldebug) $(lflags) /libpath:..\libdispatch\bin\$(PLATFORM)\$(CONFIGURATION) -out:$(OUTDIR)dispatch_starfish.exe $** $(guilibsdll) libdispatch.lib

dispatch_pingpong.exe: $(INTDIR)dispatch_pingpong.obj $(INTDIR)dispatch_test.obj
	$(link) $(ldebug) $(lflags) /libpath:..\libdispatch\bin\$(PLATFORM)\$(CONFIGURATION) -out:$(OUTDIR)dispatch_pingpong.exe $** $(guilibsdll) libdispatch.lib

dispatch_debug.exe: $(INTDIR)dispatch_debug.obj $(INTDIR)dispatch_test.obj
	$(link) $(ldebug) $(lflags) /libpath:..\libdispatch\bin\$(PLATFORM)\$(CONFIGURATION) -out:$(OUTDIR)dispatch_debug.exe $** $(guilibsdll) libdispatch.lib

dispatch_timer_set_time.exe: $(INTDIR)dispatch_timer_set_time.obj $(INTDIR)dispatch_test.obj
	$(link) $(ldebug) $(lflags) /libpath:..\libdispatch\bin\$(PLATFORM)\$(CONFIGURATION) -out:$(OUTDIR)dispatch_timer_set_time.exe $** $(guilibsdll) libdispatch.lib

dispatch_readsync.exe: $(INTDIR)dispatch_readsync.obj $(INTDIR)dispatch_test.obj
	$(link) $(ldebug) $(lflags) /libpath:..\libdispatch\bin\$(PLATFORM)\$(CONFIGURATION) -out:$(OUTDIR)dispatch_readsync.exe $** $(guilibsdll) libdispatch.lib

dispatch_plusplus.exe: $(INTDIR)dispatch_plusplus.obj $(INTDIR)dispatch_test.obj
	$(link) $(ldebug) $(lflags) /libpath:..\libdispatch\bin\$(PLATFORM)\$(CONFIGURATION) /libpath:"..\libdispatch++\bin\$(PLATFORM)\$(CONFIGURATION)" -out:$(OUTDIR)dispatch_plusplus.exe $** $(guilibsdll) libdispatch.lib "libdispatch++.lib"

dispatch_windows.exe: $(INTDIR)dispatch_windows.obj $(INTDIR)dispatch_test.obj
	$(link) $(ldebug) $(lflags) $(guilflags) /INCREMENTAL:NO /NOLOGO /OPT:ICF /libpath:..\libdispatch\bin\$(PLATFORM)\$(CONFIGURATION) /libpath:"..\libdispatch++\bin\$(PLATFORM)\$(CONFIGURATION)" -out:$(OUTDIR)dispatch_windows.exe $** $(guilibsdll) libdispatch.lib "libdispatch++.lib"

rebuild: clean all

clean:
	$(CLEANUP)
