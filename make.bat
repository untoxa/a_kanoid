@echo off
@set PROJ=a_kanoid
@set GBDK=..\..\gbdk
@set GBDKLIB=%GBDK%\lib\small\asxxxx
@set OBJ=build\
@set SRC=src\

@set CFLAGS=-mgbz80 --no-std-crt0 -I %GBDK%\include -I %GBDK%\include\asm -I %SRC%include -c

@set LFLAGS=-n -- -z -m -j -yt2 -yo4 -ya4 -k%GBDKLIB%\gbz80\ -lgbz80.lib -k%GBDKLIB%\gb\ -lgb.lib 
@set LFILES=%GBDKLIB%\gb\crt0.o

@set ASMFLAGS=-plosgff -I"libc"

@echo Cleanup...

@if exist %OBJ% @rd /s/q %OBJ%
@if exist %PROJ%.gb @del %PROJ%.gb
@if exist %PROJ%.sym @del %PROJ%.sym
@if exist %PROJ%.map @del %PROJ%.map

@if not exist %OBJ% mkdir %OBJ%

@echo ASSEMBLING THE STUB...

sdasgb %ASMFLAGS% %OBJ%MBC1_RAM_INIT.rel %SRC%MBC1_RAM_INIT.s
@set LFILES=%LFILES% %OBJ%MBC1_RAM_INIT.rel

@echo COMPILING WITH SDCC4...

sdcc %CFLAGS% %SRC%threads.c -o %OBJ%threads.rel
sdcc %CFLAGS% %SRC%ring.c -o %OBJ%ring.rel
sdcc %CFLAGS% %SRC%utils.c -o %OBJ%utils.rel
sdcc %CFLAGS% %SRC%sprite_utils.c -o %OBJ%sprite_utils.rel
@set LFILES=%LFILES% %OBJ%sprite_utils.rel %OBJ%threads.rel %OBJ%ring.rel %OBJ%utils.rel

sdcc %CFLAGS% %SRC%globals.c -o %OBJ%globals.rel
sdcc %CFLAGS% %SRC%ball.c -o %OBJ%ball.rel
sdcc %CFLAGS% %SRC%tile_data.c -o %OBJ%tile_data.rel
@set LFILES=%LFILES% %OBJ%globals.rel %OBJ%ball.rel %OBJ%tile_data.rel

sdcc %CFLAGS% %SRC%%PROJ%.c -o %OBJ%%PROJ%.rel

@echo LINKING WITH GBDK...
%GBDK%\bin\link-gbz80 %LFLAGS% %PROJ%.gb %LFILES% %OBJ%%PROJ%.rel 

@echo DONE!
