@echo off
REM Set the path to vcpkg
set "vcpkg_path=D:/workspace/vcpkg"

echo Starting the build process...

REM Step 1: Create the build directory if it doesn't exist
if not exist "build" (
    mkdir build
)

REM Navigate to the build directory
cd build

REM Step 2: Run CMake to configure the project
cmake .. -DCMAKE_TOOLCHAIN_FILE=%vcpkg_path%/scripts/buildsystems/vcpkg.cmake ^
          -DVCPKG_TARGET_TRIPLET=x86-windows-static ^
          -DCMAKE_BUILD_TYPE=Release

REM Step 3: Build the project
cmake --build . --config Release

echo Build process completed.
pause