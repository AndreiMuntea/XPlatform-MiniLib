
echo Generating gtest library for SolutionDir %1 Configuration %2 and Platform %3 

set VS_IDE_PATH=%4
echo Visual Studio IDE: %VS_IDE_PATH%

set VS_DEVENV_PATH=%VS_IDE_PATH:~0,-1%devenv.exe"
echo Visual Studio devenev: %VS_DEVENV_PATH%

set VS_CMAKE_PATH=%VS_IDE_PATH:~0,-1%CommonExtensions\Microsoft\Cmake\Cmake\bin\cmake.exe"
echo CMake Path: %VS_CMAKE_PATH%

set GOOGLE_TEST_PATH=%1
set GOOGLE_TEST_PATH=%GOOGLE_TEST_PATH:~0,-1%lib\googletest"
set GOOGLE_TEST_OUT_PATH=%GOOGLE_TEST_PATH:~0,-1%\out"
echo GoogleTest library Path: %GOOGLE_TEST_PATH%

echo Cleaning output directory %GOOGLE_TEST_OUT_PATH%
del /s /f /q %GOOGLE_TEST_OUT_PATH%

echo %VS_CMAKE_PATH% -S %GOOGLE_TEST_PATH% -B %GOOGLE_TEST_OUT_PATH% -A %3 -DCMAKE_BUILD_TYPE=%2
%VS_CMAKE_PATH% -S %GOOGLE_TEST_PATH% -B %GOOGLE_TEST_OUT_PATH% -A %3 -DCMAKE_BUILD_TYPE=%2

set GOOGLE_TEST_LIB_PATH=%GOOGLE_TEST_OUT_PATH:~0,-1%\lib\%2\*.*"
set GOOGLE_TEST_SLN_PATH=%GOOGLE_TEST_OUT_PATH:~0,-1%\googletest-distribution.sln"

echo GTEST Output Directories:
echo     out: %GOOGLE_TEST_OUT_PATH%
echo     lib: %GOOGLE_TEST_LIB_PATH%
echo     sln: %GOOGLE_TEST_SLN_PATH%

echo Running %VS_DEVENV_PATH% %GOOGLE_TEST_SLN_PATH% /Rebuild "%2|%3"
%VS_DEVENV_PATH% %GOOGLE_TEST_SLN_PATH% /Rebuild "%2|%3"

set GTEST_DESTINATION_LIB_PATH=%1
set GTEST_DESTINATION_LIB_PATH=%GTEST_DESTINATION_LIB_PATH:~0,-1%\XPF-Tests\gtest_libs"
echo Target GTEST Library Directory %GTEST_DESTINATION_LIB_PATH%

echo Copying gtest libraries xcopy %GOOGLE_TEST_LIB_PATH% %GTEST_DESTINATION_LIB_PATH% /K /D /H /Y
xcopy %GOOGLE_TEST_LIB_PATH% %GTEST_DESTINATION_LIB_PATH% /K /D /H /Y