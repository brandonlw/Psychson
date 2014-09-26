@ECHO OFF

REM Set things up and create bin directory if necessary.
SETLOCAL ENABLEDELAYEDEXPANSION
SET BUILD_FILES=
IF NOT EXIST bin\NUL MKDIR bin

REM Build each file in the list.
FOR %%A IN (
main
timers
usb
control
scsi
) DO (
ECHO *** Building %%A.c...
sdcc --model-small -mmcs51 -pdefcpu -c -obin\%%A.rel %%A.c
IF ERRORLEVEL 1 GOTO ERRORS
SET "BUILD_FILES=!BUILD_FILES! bin\%%A.rel"
)

REM Build Intel Hex and BIN versions of combined file.
sdcc --xram-loc 0x6000 -o bin\output.hex %BUILD_FILES%
IF ERRORLEVEL 1 GOTO ERRORS
makebin -p bin\output.hex bin\output.bin

REM Create firmware and burner images from templates.
copy /y ..\templates\FWdummy.bin bin\fw.bin > NUL
copy /y ..\templates\BNdummy.bin bin\bn.bin > NUL
..\tools\sfk partcopy bin\output.bin -fromto 0 -1 bin\fw.bin 512 -yes > NUL
..\tools\sfk partcopy bin\output.bin -fromto 0 -1 bin\bn.bin 512 -yes > NUL

GOTO END

:ERRORS
ECHO *** There were errors^^! ***

:END
ECHO *** Done.

ENDLOCAL
