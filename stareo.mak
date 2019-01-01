.AUTODEPEND

#		*Translator Definitions*
CC = bcc +STAREO.CFG
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
 decoder.obj \
 stereo3.obj \
 escher.obj

#		*Explicit Rules*
stareo.exe: stareo.cfg $(EXE_dependencies)
  $(TLINK) /v/x/c/P-/L$(LIBPATH) @&&|
c0l.obj+
decoder.obj+
stereo3.obj+
escher.obj
stareo
		# no map file
graphics.lib+
emu.lib+
mathl.lib+
cl.lib
|


#		*Individual File Dependencies*
decoder.obj: stareo.cfg decoder.cpp 

stereo3.obj: stareo.cfg stereo3.cpp 

escher.obj: stareo.cfg escher.cpp 

#		*Compiler Configuration File*
stareo.cfg: stareo.mak
  copy &&|
-ml
-v
-vi-
-H=STAREO.SYM
-wpro
-weas
-wpre
-I$(INCLUDEPATH)
-L$(LIBPATH)
| stareo.cfg


