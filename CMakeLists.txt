# CMakeList.txt: 顶层 CMake 项目文件，在此处执行全局配置
# 并包含子项目。
#
cmake_minimum_required (VERSION 3.0)

# SET(CMAKE_TOOLCHAIN_FILE "E:/git_pro/vcpkg/scripts/buildsystems/vcpkg.cmake")
# SET(VCPKG_TARGET_TRIPLET x86-windows-static)

# set(VCPKG_INSTALL_PREFIX ${CMAKE_CURRENT_SOURCE_DIR})


project ("op")

# 包含子项目。
add_subdirectory ("libop")

add_subdirectory("com")
add_subdirectory("python")
add_subdirectory("lua")

add_subdirectory("tools")
add_subdirectory("tests")

