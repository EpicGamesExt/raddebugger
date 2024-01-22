@echo off
setlocal
cd /D "%~dp0"

png2ico raddebugger.ico raddebugger.png
rc /v icon.rc