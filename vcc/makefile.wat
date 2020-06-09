
CC = wpp386
CFLAGS = /u_WIN32 /oneatx /zp4 /j /d0 /wx

OBJ = compile.obj funclib.obj lexical.obj linked.obj memstr.obj &
      preproc.obj str.obj vcc.obj

.CC: watcom

.CC.OBJ:
        $(CC) $(CFLAGS) $[*
        wlib vcc.lib +-$^&

.CC.EXE:
        wcl386 /bt=dos /l=dos4g /k60000 $(CFLAGS) $[* vcc.lib mikwat.lib

.ASM: watcom

.ASM.OBJ:
        wasm $[*
        wlib vcc.lib +-$^&

vcc.exe: $(OBJ)
