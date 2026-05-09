@echo off
setlocal enabledelayedexpansion

set TARGET_VALUES=radlink radbin mule_main mule_module torture
set CC_VALUES=msvc clang
set MODE_VALUES=debug release
set RAW_ARGS=%*
set TORTURE_ARGS=
set RUN_TORTURE=1

:parse_args
if "%~1" == ""   goto parse_done
if "%~1" == "--" goto parse_torture_args
if /i "%~1" == "msvc"       set CC_VALUES=msvc      && shift /1 && goto parse_args
if /i "%~1" == "clang"      set CC_VALUES=clang     && shift /1 && goto parse_args
if /i "%~1" == "debug"      set MODE_VALUES=debug   && shift /1 && goto parse_args
if /i "%~1" == "release"    set MODE_VALUES=release && shift /1 && goto parse_args
if /i "%~1" == "no_torture" set RUN_TORTURE=0       && shift /1 && goto parse_args
echo usage: %~nx0 [msvc^|clang] [debug^|release] [no_torture] [-- torture-args]
exit /b 1

:parse_torture_args
shift /1
if "%~1" == "" set "TORTURE_ARGS=" && goto parse_done
set "TORTURE_ARGS=%RAW_ARGS%"
if "%TORTURE_ARGS:~0,3%" == "-- " set "TORTURE_ARGS=%TORTURE_ARGS:~3%" && goto parse_done
set "TORTURE_ARGS=%TORTURE_ARGS:* -- =%"

:parse_done

for %%m in (%MODE_VALUES%) do for %%c in (%CC_VALUES%) do (
  setlocal

  :: nuke artifacts from last run
  if exist build/torture_artifacts rmdir /s /q build\torture_artifacts
  if exist build\metagen.exe del /q build\metagen.exe
  for %%t in (%TARGET_VALUES%) do (
    if exist build\%%t.exe del /q build\%%t.exe
    if exist build\%%t.pdb del /q build\%%t.pdb
  )

  if "%%c" == "clang" (
    :: Prefer standalone LLVM clang over Visual Studio clang so ASAN runtime files match the compiler.
    set "clang_path="
    for /f "tokens=*" %%i in ('where clang') do (
      set "candidate_clang_path=%%~dpi"
      if "!candidate_clang_path:Microsoft Visual Studio=!"=="!candidate_clang_path!" (
        if not defined clang_path set "clang_path=!candidate_clang_path!"
      )
    )
    if not defined clang_path (
      echo ERROR: standalone LLVM clang not found in PATH
      exit /b 1
    )
    set PATH=!clang_path!;!PATH!
    for /f "tokens=3 delims=. " %%v in ('clang -v 2^>^&1 ^| findstr version') do set clang_version=%%v
    if not defined clang_version (
      echo ERROR: failed to detect clang version
      exit /b 1
    )
    set PATH=!clang_path!..\lib\clang\!clang_version!\lib\windows;!PATH!
  )

  :: TODO: unblock asan
  call build.bat meta %%c %%m raddbg raddbg_non_graphical || (endlocal exit /b 1)

  :: clang does not compile with asan in release mode because it runs out of memory
  if "%%c" equ "clang" (
    if /i "%%m" equ "release" (
      call build.bat no_meta %%c %%m !TARGET_VALUES! || (endlocal exit /b 1)
    ) else (
      call build.bat asan no_meta %%c %%m !TARGET_VALUES! || (endlocal exit /b 1)
    )
  ) else (
    call build.bat asan no_meta %%c %%m !TARGET_VALUES! || (endlocal exit /b 1)
  )

  if "%RUN_TORTURE%" equ "1" (
    pushd build
    torture %TORTURE_ARGS%
    popd
    if errorlevel 1 (
      endlocal
      exit /b 1
    )
  )

  endlocal
)
