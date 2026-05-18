@echo off
setlocal

gcc -Wall -Wextra -g main.c decoder.c execution.c flags.c hazards.c memory.c parser.c pipeline.c print.c registers.c -o simulator.exe
if errorlevel 1 exit /b 1

gcc -Wall -Wextra -g gui.c -mwindows -o simulator_gui.exe
if errorlevel 1 exit /b 1

echo Built simulator.exe and simulator_gui.exe
