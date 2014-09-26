@ECHO OFF

REM Set things up and create bin directory if necessary.
SETLOCAL ENABLEDELAYEDEXPANSION
SET BUILD_FILES=
IF NOT EXIST bin\NUL MKDIR bin

REM Generate .h C file for compilation.
ECHO *** Generating C .h file...
..\tools\Injector.exe /action=GenerateHFile /firmware=fw.bin /output=equates.h
IF ERRORLEVEL 1 GOTO ERRORS

REM Build each file in the list.
REM NOTE: This needs to change if more code files or sections are added.
FOR %%A IN (
base
) DO (
ECHO *** Building %%A.c...
sdcc --model-small -mmcs51 -pdefcpu -c -obin\%%A.rel %%A.c
IF ERRORLEVEL 1 GOTO ERRORS
SET "BUILD_FILES=!BUILD_FILES! bin\%%A.rel"
)

REM Retrieve free space for each section in the image.
ECHO *** Retrieving free space in image...
..\tools\Injector.exe /action=FindFreeBlock /firmware=fw.bin /section=Base /output=bin\free.txt
SET BASE_FREE_ADDR=
FOR /F "delims=" %%i IN (bin\free.txt) DO SET BASE_FREE_ADDR=!BASE_FREE_ADDR! %%i
DEL bin\free.txt

REM Build Intel Hex and BIN versions of combined file.
ECHO *** Linking...
sdcc --model-small --code-loc %BASE_FREE_ADDR% --xram-size 0x400 --xram-loc 0x7C00 -o bin\output.hex %BUILD_FILES%
..\tools\hex2bin bin\output.hex

REM Build patched image from assembled image.
REM NOTE: This needs to change if more code files or sections are added.
ECHO *** Injecting...
..\tools\Injector.exe /action=ApplyPatches /firmware=fw.bin /basecode=bin\output.bin /baserst=bin\base.rst /output=bin\fw.bin
IF ERRORLEVEL 1 GOTO ERRORS

GOTO END

:ERRORS
ECHO *** There were errors^^! ***

:END
ECHO *** Done.

ENDLOCAL
