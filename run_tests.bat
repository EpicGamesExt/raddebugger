@echo off
setlocal enabledelayedexpansion

rem Make sure that your VS install does not have LLVM included otherwise this script will use VS clang

set TARGET_VALUES=radlink radbin mule_main mule_module torture
set CC_VALUES=msvc clang
set MODE_VALUES=debug release

rem find path to clang
for /f "tokens=*" %%i in ('where clang') do set clang_path=%%~dpi

rem parse out clang version number out of --version to build path to the folder with ASAN DLL
for /f "tokens=3 delims=. " %%v in ('clang -v 2^>^&1 ^| findstr version') do set clang_version=%%v

for %%m in (%MODE_VALUES%) do for %%c in (%CC_VALUES%) do (
  setlocal

  echo --------------------------------------------------------------------------------
  echo Build %%c+%%m

  rem nuke artifacts from last run
  rmdir /s /q build\torture

  if "%%c" equ "clang" (
     set PATH=%clang_path%..\lib\clang\%clang_version%\lib\windows;!PATH!
  )

  rem TODO: unblock asan
  call build.bat meta %%c %%m raddbg_non_graphical || exit /b 1

  rem clang does not compile with asan in release mode because it runs out of memory
  if "%%c" equ "clang" (
    call build.bat meta %%c %%m %TARGET_VALUES% || exit /b 1
  ) else (
    call build.bat meta asan %%c %%m %TARGET_VALUES% || exit /b 1
  )

  pushd build
  torture %* || exit /b 1
  popd

  endlocal
)

