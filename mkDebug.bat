@ECHO OFF
call "C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvars64.bat" amd64
Title Debug
:BEGIN
mkdir build
cd build
mkdir Debug
cd Debug
cmake -DCMAKE_BUILD_TYPE=RelWithDebInfo  ..\..\ -G"NMake Makefiles"
nmake
cd ..\..\
echo %cd%
rem pause
rem goto :BEGIN
echo ========================OK====================