@echo off
setlocal enabledelayedexpansion
cd /D "%~dp0"
:restart

:: --- Usage Notes (2024/1/10) ------------------------------------------------
::
:: This is a central build script for the RAD Debugger project, for use in
:: Windows development environments. It takes a list of simple alphanumeric-
:: only arguments which control (a) what is built, (b) which compiler & linker
:: are used, and (c) extra high-level build options. By default, if no options
:: are passed, then the main "raddbg" graphical debugger is built.
::
:: Below is a non-exhaustive list of possible ways to use the script:
:: `build raddbg`
:: `build raddbg clang`
:: `build raddbg release`
:: `build raddbg asan telemetry`
:: `build rdi_from_pdb`
::
:: For a full list of possible build targets and their build command lines,
:: search for @build_targets in this file.
::
:: Below is a list of all possible non-target command line options:
::
:: - `asan`: enable address sanitizer
:: - `telemetry`: enable RAD telemetry profiling support
:: - `spall`: enable spall profiling support

:: --- Unpack Arguments -------------------------------------------------------
for %%a in (%*) do set "%%~a=1"
if not "%msvc%"=="1" if not "%clang%"=="1" set msvc=1
if not "%release%"=="1" set debug=1
if "%debug%"=="1"   set release=0 && echo [debug mode]
if "%release%"=="1" set debug=0 && echo [release mode]
if "%msvc%"=="1"    set clang=0 && echo [msvc compile]
if "%clang%"=="1"   set msvc=0 && echo [clang compile]
if "%~1"==""                     echo [default mode, assuming `raddbg` build] && set raddbg=1
if "%~1"=="release" if "%~2"=="" echo [default mode, assuming `raddbg` build] && set raddbg=1

:: --- Unpack Command Line Build Arguments ------------------------------------
set auto_compile_flags=
if "%telemetry%"=="1"               set auto_compile_flags=%auto_compile_flags% -DPROFILE_TELEMETRY=1 && echo [telemetry profiling enabled]
if "%spall%"=="1"                   set auto_compile_flags=%auto_compile_flags% -DPROFILE_SPALL=1 && echo [spall profiling enabled]
if "%asan%"=="1"                    set auto_compile_flags=%auto_compile_flags% -fsanitize=address && echo [asan enabled]
if "%opengl%"=="1"                  set auto_compile_flags=%auto_compile_flags% -DR_BACKEND=R_BACKEND_OPENGL && echo [opengl render backend]
if "%dwarf%"=="1" if "%clang%"=="1" set auto_compile_flags=%auto_compile_flags% -gdwarf && echo [dwarf debug info]
if "%dwarf%"==""  if "%clang%"=="1" set auto_compile_flags=%auto_compile_flags% -gcodeview
if "%pgo%"=="1" (
  if "%no_meta%"=="" echo ERROR: PGO build must have no_meta argument || exit /b 1
  where llvm-profdata /q || echo llvm-profdata is not in the PATH || exit /b 1 
  if "%clang%"=="1" (
    if "%pgo_run%" == "1" (
      call llvm-profdata merge %LLVM_PROFILE_FILE% -output=%~dp0build\build.profdata || exit /b 1
      set auto_compile_flags=%auto_compile_flags% -fprofile-use=%~dp0build\build.profdata
      set pgo_run=0
    ) else (
      echo [pgo enabled]
      set auto_compile_flags=%auto_compile_flags% -fprofile-generate -mllvm -vp-counters-per-site=5
      set LLVM_PROFILE_FILE=%~dp0build\build.profraw
      set pgo_run=1
    )
  ) else (
    echo ERROR: PGO build is not supported with current compiler
    exit /b 1
  )
)

:: --- Compile/Link Line Definitions ------------------------------------------
set cl_common=     /I..\src\ /I..\local\ /nologo /FC /Z7
set cl_debug=      call cl /Od /Ob1 /DBUILD_DEBUG=1 %cl_common% %auto_compile_flags%
set cl_release=    call cl /O2 /DBUILD_DEBUG=0 %cl_common% %auto_compile_flags%
set cl_link=       /link /MANIFEST:EMBED /INCREMENTAL:NO /pdbaltpath:%%%%_PDB%%%% /NATVIS:"%~dp0\src\natvis\base.natvis" /noexp /nocoffgrpinfo /opt:ref /opt:icf
set cl_out=        /out:
set cl_linker=     
set clang_common=  -I..\src\ -I..\local\ -fdiagnostics-absolute-paths -Wall -Wno-unknown-warning-option -Wno-missing-braces -Wno-unused-function -Wno-unused-parameter -Wno-writable-strings -Wno-missing-field-initializers -Wno-unused-value -Wno-unused-variable -Wno-unused-local-typedef -Wno-deprecated-register -Wno-deprecated-declarations -Wno-unused-but-set-variable -Wno-single-bit-bitfield-constant-conversion -Wno-compare-distinct-pointer-types -Wno-initializer-overrides -Wno-incompatible-pointer-types-discards-qualifiers -Xclang -flto-visibility-public-std -D_USE_MATH_DEFINES -Dstrdup=_strdup -Dgnu_printf=printf -ferror-limit=10000 -mcx16 -msha
set clang_debug=   call clang -g -O0 -DBUILD_DEBUG=1 %clang_common% %auto_compile_flags%
set clang_release= call clang -g -O2 -DBUILD_DEBUG=0 %clang_common% %auto_compile_flags%
set clang_link=    -fuse-ld=lld -Xlinker /MANIFEST:EMBED -Xlinker /pdbaltpath:%%%%_PDB%%%% -Xlinker /NATVIS:"%~dp0\src\natvis\base.natvis" -Xlinker /opt:ref -Xlinker /opt:icf
set clang_out=     -o
set clang_linker=  -Xlinker

:: --- Per-Build Settings -----------------------------------------------------
set link_dll=-DLL
set link_icon=logo.res
if "%msvc%"=="1"    set linker=%cl_linker%
if "%clang%"=="1"   set linker=%clang_linker%
if "%msvc%"=="1"    set only_compile=/c
if "%clang%"=="1"   set only_compile=-c
if "%msvc%"=="1"    set EHsc=/EHsc
if "%clang%"=="1"   set EHsc=
if "%msvc%"=="1"    set no_aslr=/DYNAMICBASE:NO
if "%clang%"=="1"   set no_aslr=-Wl,/DYNAMICBASE:NO
if "%msvc%"=="1"    set rc=call rc
if "%clang%"=="1"   set rc=call llvm-rc

:: --- Choose Compile/Link Lines ----------------------------------------------
if "%msvc%"=="1"      set compile_debug=%cl_debug%
if "%msvc%"=="1"      set compile_release=%cl_release%
if "%msvc%"=="1"      set compile_link=%cl_link%
if "%msvc%"=="1"      set out=%cl_out%
if "%clang%"=="1"     set compile_debug=%clang_debug%
if "%clang%"=="1"     set compile_release=%clang_release%
if "%clang%"=="1"     set compile_link=%clang_link%
if "%clang%"=="1"     set out=%clang_out%
if "%debug%"=="1"     set compile=%compile_debug%
if "%release%"=="1"   set compile=%compile_release%

:: --- Prep Directories -------------------------------------------------------
if not exist build mkdir build
if not exist local mkdir local

:: --- Produce Logo Icon File -------------------------------------------------
pushd build
%rc% /nologo /fo logo.res ..\data\logo.rc || exit /b 1
popd

:: --- Get Current Git Commit Id ----------------------------------------------
for /f %%i in ('call git describe --always --dirty')   do set compile=%compile% -DBUILD_GIT_HASH=\"%%i\"
for /f %%i in ('call git rev-parse HEAD')              do set compile=%compile% -DBUILD_GIT_HASH_FULL=\"%%i\"

:: --- Build & Run Metaprogram ------------------------------------------------
if "%no_meta%"=="1" echo [skipping metagen]
if not "%no_meta%"=="1" (
  pushd build
  %compile_debug% ..\src\metagen\metagen_main.c %compile_link% %out%metagen.exe || exit /b 1
  metagen.exe || exit /b 1
  popd
)

:: --- Build Everything (@build_targets) --------------------------------------
pushd build
if "%raddbg%"=="1"                     set didbuild=1 && %compile% ..\src\raddbg\raddbg_main.c                               %compile_link% %link_icon% %out%raddbg.exe || exit /b 1
if "%radlink%"=="1"                    set didbuild=1 && %compile% ..\src\linker\lnk.c                                       %compile_link% %linker% /NOIMPLIB %linker% /NATVIS:"%~dp0\src\linker\linker.natvis" %out%radlink.exe || exit /b 1
if "%radbin%"=="1"                     set didbuild=1 && %compile% ..\src\radbin\radbin_main.c                               %compile_link% %out%radbin.exe || exit /b 1
if "%raddump%"=="1"                    set didbuild=1 && %compile% ..\src\raddump\raddump_main.c                             %compile_link% %out%raddump.exe || exit /b 1
if "%tester%"=="1"                     set didbuild=1 && %compile% ..\src\tester\tester_main.c                               %compile_link% %out%tester.exe || exit /b 1
if "%ryan_scratch%"=="1"               set didbuild=1 && %compile% ..\src\scratch\ryan_scratch.c                             %compile_link% %out%ryan_scratch.exe || exit /b 1
if "%eval_scratch%"=="1"               set didbuild=1 && %compile% ..\src\scratch\eval_scratch.c                             %compile_link% %out%eval_scratch.exe || exit /b 1
if "%textperf%"=="1"                   set didbuild=1 && %compile% ..\src\scratch\textperf.c                                 %compile_link% %out%textperf.exe || exit /b 1
if "%convertperf%"=="1"                set didbuild=1 && %compile% ..\src\scratch\convertperf.c                              %compile_link% %out%convertperf.exe || exit /b 1
if "%debugstringperf%"=="1"            set didbuild=1 && %compile% ..\src\scratch\debugstringperf.c                          %compile_link% %out%debugstringperf.exe || exit /b 1
if "%parse_inline_sites%"=="1"         set didbuild=1 && %compile% ..\src\scratch\parse_inline_sites.c                       %compile_link% %out%parse_inline_sites.exe || exit /b 1
if "%strip_lib_debug%"=="1"            set didbuild=1 && %compile% ..\src\strip_lib_debug\strip_lib_debug.c                  %compile_link% %out%strip_lib_debug.exe || exit /b 1
if "%mule_main%"=="1"                  set didbuild=1 && del vc*.pdb mule*.pdb && %compile_release% %only_compile% ..\src\mule\mule_inline.cpp && %compile_release% %only_compile% ..\src\mule\mule_o2.cpp && %compile_debug% %EHsc% ..\src\mule\mule_main.cpp ..\src\mule\mule_c.c mule_inline.obj mule_o2.obj %compile_link% %no_aslr% %out%mule_main.exe || exit /b 1
if "%mule_module%"=="1"                set didbuild=1 && %compile% ..\src\mule\mule_module.cpp                               %compile_link% %link_dll% %out%mule_module.dll || exit /b 1
if "%mule_hotload%"=="1"               set didbuild=1 && %compile% ..\src\mule\mule_hotload_main.c %compile_link% %out%mule_hotload.exe & %compile% ..\src\mule\mule_hotload_module_main.c %compile_link% %link_dll% %out%mule_hotload_module.dll || exit /b 1
if "%torture%"=="1"                    set didbuild=1 && %compile% ..\src\torture\torture.c                                  %compile_link% %out%torture.exe || exit /b1
if "%mule_peb_trample%"=="1" (
  set didbuild=1
  if exist mule_peb_trample.exe move mule_peb_trample.exe mule_peb_trample_old_%random%.exe
  if exist mule_peb_trample_new.pdb move mule_peb_trample_new.pdb mule_peb_trample_old_%random%.pdb
  if exist mule_peb_trample_new.rdi move mule_peb_trample_new.rdi mule_peb_trample_old_%random%.rdi
  %compile% ..\src\mule\mule_peb_trample.c %compile_link% %out%mule_peb_trample_new.exe || exit /b 1
  move mule_peb_trample_new.exe mule_peb_trample.exe
)
popd

:: --- Warn On No Builds ------------------------------------------------------
if "%didbuild%"=="" (
  echo [WARNING] no valid build target specified; must use build target names as arguments to this script, like `build raddbg` or `build rdi_from_pdb`.
  exit /b 1
)

:: --- PGO Run ----------------------------------------------------------------
if "%pgo_run%"=="1" (
  if "%radlink%"=="1" (
    pushd local\lyra_pgo
    call %~dp0build\radlink @lyra.rsp /rad_alt_pch_dir:%~dp0local\lyra_pgo || exit /b 1
    popd
  )
  goto restart
)
