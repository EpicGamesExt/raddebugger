@echo off
setlocal
cd /D "%~dp0"

echo --- getting test data folder path ---------------------------------------------
if not exist .\local\test_data_path.txt (
  echo error: You must first store the full path of your test data folder inside of `local/test_data_path.txt`.
  goto :EOF
)
set /p test_data_folder=<.\local\test_data_path.txt	
echo test data path: %test_data_folder%
echo:

echo --- building all testing executables ------------------------------------------
call build rdi_from_pdb rdi_dump raddbg radlink tester
echo:

echo --- running tests -------------------------------------------------------------
pushd build
call tester.exe --test_data:%test_data_folder%
popd
