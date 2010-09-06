@echo off

echo.
echo Testing 'echo on/off' and at-sign ...
echo.
echo on
; I am a comment
@echo off
echo.
pause 5000

echo.
echo Testing 'type' ...
echo.
type
echo.
pause 3000

echo.
echo Testing 'type {file name}' ...
echo.
type C:\ud-boot-time-test-call.cmd
echo.
pause 3000

echo.
echo Testing 'man' ...
echo.
man
echo.
pause 3000

echo.
echo Testing 'man {command}' ...
echo.
man variables
echo.
pause 3000

echo.
echo Testing 'help' ...
echo.
help
echo.
pause 3000

echo.
echo Testing 'set' ...
echo.
set
echo.
pause 3000

echo.
echo Testing 'set {letter}' ...
echo.
set p
echo.
pause 3000

echo.
echo Testing 'set {variable=value}' ...
echo.
set z
set ZYXEL=Eureka, it works
set z
echo.
pause 3000

echo.
echo Testing 'history' ...
echo.
history
echo.
pause 3000

echo.
echo Testing 'call {file name}' ...
echo.
call C:\ud-boot-time-test-call.cmd
echo.
pause 3000
