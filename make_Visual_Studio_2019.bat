@ECHO OFF
call "C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvarsall.bat" amd64
Title Debug
:BEGIN
mkdir build
cd build
mkdir msvc16
cd msvc16
set CMAKE_PREFIX_PATH=K:\Qt5\lib\cmake
cmake  -DCMAKE_BUILD_TYPE=RelWithDebInfo  ..\..\ -G"Visual Studio 16 2019"
@echo msbuild Project.sln /p:Configuration=Debug /p:Platform=X64 /m
cd ..\..\
echo %cd%
echo ========================OK====================
pause
goto BEGIN