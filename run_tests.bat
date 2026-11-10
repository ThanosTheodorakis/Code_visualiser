@echo off
rem run_tests.bat - Builds the project, runs all tests and saves the results in test_results.txt.

set GEN=%~1
if not "%GEN%"=="" goto have_gen
set VSW="%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
if exist %VSW% for /f "tokens=1 delims=." %%V in ('%VSW% -latest -property installationVersion') do set VSMAJOR=%%V
if "%VSMAJOR%"=="16" set GEN=Visual Studio 16 2019
if "%VSMAJOR%"=="17" set GEN=Visual Studio 17 2022
if "%VSMAJOR%"=="18" set GEN=Visual Studio 18 2026
:have_gen
if "%GEN%"=="" (
    echo Could not tell which Visual Studio is installed. Run: run_tests.bat "Visual Studio NN YYYY"
    echo ^(use a name from the list at the end of "cmake --help"^)
    exit /b 1
)
echo Using generator: %GEN%
cmake -S app -B build -G "%GEN%" -A x64 || exit /b 1
cmake --build build --config Release || exit /b 1

echo Code_visualiser test run > test_results.txt
echo %DATE% %TIME% >> test_results.txt
ver >> test_results.txt
cmake --version | findstr /b cmake >> test_results.txt
echo. >> test_results.txt

pushd build
echo === ctest === >> ..\test_results.txt
ctest -C Release --output-on-failure >> ..\test_results.txt 2>&1
echo. >> ..\test_results.txt
echo === speed_test === >> ..\test_results.txt
tests\Release\speed_test.exe >> ..\test_results.txt 2>&1
echo. >> ..\test_results.txt
echo === each test on its own === >> ..\test_results.txt
for %%T in (demo_test markers_test backlog_test input_test shapes_test runfile_test eventlog_test) do (
    tests\Release\%%T.exe >> ..\test_results.txt 2>&1
)
popd

type test_results.txt
