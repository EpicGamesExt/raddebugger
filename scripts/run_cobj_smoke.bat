@echo off
setlocal
where cl >nul 2>nul || (
  echo Run this script from an x64 Visual Studio developer command prompt
  exit /b 1
)
pushd "%~dp0..\build"
cl /nologo /c /Z7 /Od "%~dp0cobj_smoke.c" /Focobj_smoke_raw.obj || exit /b 1
rad_obj_compress.exe cobj_smoke_raw.obj cobj_smoke_compressed.obj 512 selkie || exit /b 1
radlink.exe /nologo /debug:ghash /pdb:cobj_smoke.pdb /entry:main /subsystem:console /out:cobj_smoke_raw.exe cobj_smoke_raw.obj || exit /b 1
set RAD_COBJ_CACHE_MIB=16
set RAD_COBJ_SKIP_CLEANUP=1
radlink.exe /nologo /debug:ghash /pdb:cobj_smoke.pdb /entry:main /subsystem:console /out:cobj_smoke_compressed.exe cobj_smoke_compressed.obj || exit /b 1
fc /b cobj_smoke_raw.exe cobj_smoke_compressed.exe || exit /b 1
popd
