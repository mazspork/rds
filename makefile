.AUTODEPEND

#		*Translator Definitions*
CC = bcc +STEREO.CFG
TASM = TASM
TLIB = tlib
TLINK = tlink
LIBPATH = C:\OS2\MDOS\APPS\BC\LIB
INCLUDEPATH = C:\OS2\MDOS\APPS\BC\INCLUDE


#		*Implicit Rules*
.c.obj:
  $(CC) -c {$< }

.cpp.obj:
  $(CC) -c {$< }

#		*List Macros*


EXE_dependencies =  \
 escher.obj \
 stereo3.obj

#		*Explicit Rules*
stereo.exe: stereo.cfg $(EXE_dependencies)
  $(TLINK) /v/x/c/P-/L$(LIBPATH) @&&|
c0s.obj+
escher.obj+
stereo3.obj
stereo
		# no map file
graphics.lib+
emu.lib+
maths.lib+
cs.lib
|


#		*Individual File Dependencies*
escher.obj: stereo.cfg escher.cpp 

stereo3.obj: stereo.cfg stereo3.cpp 

#		*Compiler Configuration File*
stereo.cfg: makefile
  copy &&|
-v
-vi-
-H=STEREO.SYM
-wpro
-weas
-wpre
-I$(INCLUDEPATH)
-L$(LIBPATH)
| stereo.cfg


