# CMakeList.txt: 顶层 CMake 项目文件，在此处执行全局配置
# 并包含子项目。
#
cmake_minimum_required (VERSION 3.0)

#SET(CMAKE_BUILD_TYPE "Release")
#SET(CMAKE_TOOLCHAIN_FILE "E:/git_pro/vcpkg/scripts/buildsystems/vcpkg.cmake")
SET(VCPKG_TARGET_TRIPLET x86-windows-static)
#SET(VcpkgConfiguration release)
set(VCPKG_INSTALL_PREFIX ${CMAKE_CURRENT_SOURCE_DIR})

SET(EXECUTABLE_OUTPUT_PATH ${CMAKE_CURRENT_SOURCE_DIR}/bin)
SET(LIBRARY_OUTPUT_PATH ${CMAKE_CURRENT_SOURCE_DIR}/lib)
SET(PROJECT_BINARY_DIR ${CMAKE_CURRENT_SOURCE_DIR}/bin)

#FIND_PACKAGE(Opencv REQUIRED)

project ("op")

# 包含子项目。
add_subdirectory ("op")

# add_subdirectory ("example")
