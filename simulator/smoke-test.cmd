@echo off
setlocal
cd /d "%~dp0"
set SDL_VIDEODRIVER=dummy
set SDL_AUDIODRIVER=dummy
twatch_desktop.exe --smoke > smoke-test.log 2>&1
set "result=%errorlevel%"
if "%result%"=="0" (echo PASS: all 63 screen entries rendered.) else (echo FAIL: see smoke-test.log for details.)
pause
exit /b %result%
