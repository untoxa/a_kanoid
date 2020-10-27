@echo off
@set PROJ=a_kanoid
@set GBDK=..\..\gbdk
@set GBDKLIB=%GBDK%\lib\small\asxxxx
@set OBJ=build\
@set SRC=src\

@set CFLAGS=-Isrc/include -tempdir=./build -Wl-yt2 -Wl-yo4 -Wl-ya4 -Wf"--max-allocs-per-node 50000" -Wf"--peep-file peephole\gbz80.rul" -Wl-j

@echo Cleanup...

@if exist %OBJ% @rd /s/q %OBJ%
@if exist %PROJ%.gb @del %PROJ%.gb
@if exist %PROJ%.sym @del %PROJ%.sym
@if exist %PROJ%.map @del %PROJ%.map

@if not exist %OBJ% mkdir %OBJ%

@echo COMPILING...

%GBDK%\bin\lcc %CFLAGS% -o %PROJ%.gb src/MBC1_RAM_INIT.s src/a_kanoid.c src/ball.c src/globals.c src/ring.c src/sprite_utils.c src/threads.c src/tile_data.c src/utils.c

@echo DONE!
