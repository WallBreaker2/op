echo "begin build\n"
:: 1.mkdir
mkdir build

cd build
:: 2. make
cmake .. 

:: 3. build
cmake --build . --config release

pause