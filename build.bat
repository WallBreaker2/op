
::设置vcpkg路径
set "vcpkg_path=E:/git_pro/vcpkg"
echo "begin build..."
:: 1.mkdir
mkdir build

cd build
:: 2. make
::cmake .. 
cmake .. -DCMAKE_TOOLCHAIN_FILE=%vcpkg_path%/scripts/buildsystems/vcpkg.cmake  -DVCPKG_TARGET_TRIPLET=x86-windows-static 

:: 3. build
cmake --build . --config release

echo "build finished"
pause