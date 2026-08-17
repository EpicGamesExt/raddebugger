@echo off
setlocal
if "%OODLE_SDK_DIR%"=="" (
  echo OODLE_SDK_DIR must name an Oodle SDK
  exit /b 1
)
if not exist "%OODLE_SDK_DIR%\include\oodle2.h" (
  echo Oodle SDK not found at "%OODLE_SDK_DIR%"
  exit /b 1
)
where cl >nul 2>nul || (
  echo Run this script from an x64 Visual Studio developer command prompt
  exit /b 1
)
call "%~dp0..\build.bat" radlink rad_obj_compress release oodle || exit /b 1
if not exist "%~dp0..\out_cobj" mkdir "%~dp0..\out_cobj"
copy /y "%~dp0..\build\radlink.exe" "%~dp0..\out_cobj\radlink.exe" >nul || exit /b 1
copy /y "%~dp0..\build\radlink.pdb" "%~dp0..\out_cobj\radlink.pdb" >nul || exit /b 1
copy /y "%~dp0..\build\rad_obj_compress.exe" "%~dp0..\out_cobj\rad_obj_compress.exe" >nul || exit /b 1
