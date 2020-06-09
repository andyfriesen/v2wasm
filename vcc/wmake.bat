@echo off
if exist vcc.exe del vcc.exe
wcl386 /bt=dos /l=dos4g /oneatx /wx /fe=vcc.exe vcc memstr str lexical compile funclib linked preproc
if not exist wat\vcc.exe goto end
rem if exist ..\pmwlite.exe pmwlite /c4 vcc.exe
:end
