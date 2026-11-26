@echo off
setlocal
cd /d "%~dp0"
set SDL_VIDEODRIVER=dummy
set SDL_AUDIODRIVER=dummy
twatch_desktop.exe --self-test > interaction-test.log 2>&1
set "result=%errorlevel%"
if "%result%"=="0" (echo PASS: SDL navigation and interaction checks passed.) else (echo FAIL: see interaction-test.log for details.)
pause
exit /b %result%
