@ECHO OFF
call "C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvars64.bat" amd64
Title Release
:BEGIN
mkdir build
cd build
mkdir Release
cd Release
cmake -DCMAKE_BUILD_TYPE=Release  ..\..\ -G"NMake Makefiles"
nmake
cd ..\..\
echo %cd%
echo pause
echo goto :BEGIN
echo ========================OK====================