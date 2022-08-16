# CMakeList.txt: 顶层 CMake 项目文件，在此处执行全局配置
# 并包含子项目。
#
cmake_minimum_required (VERSION 3.0)

if(NOT $ENV{VCPKG_ROOT} STREQUAL "")
    message("=====================VCPKG================")
    message($ENV{VCPKG_ROOT})
endif()

# set(VCPKG_INSTALL_PREFIX ${CMAKE_CURRENT_SOURCE_DIR})
option(build_swig_py "build swig py,requrie python" ON)
option(build_qttool "buid qt tool,requrie qt lib" OFF)

# 编译开关
set(build_qttool OFF)
set(build_swig_py OFF)

if(NOT blackbone_include_path)
    # blackbone 源码路径，必须设置！！！
    set(blackbone_include_path "E:/git_pro/Blackbone/src")
endif()

if(NOT blackbone_lib32_path)
    # blackbone 32位静态库路径，必须设置！！！
    set(blackbone_lib32_path "E:/git_pro/Blackbone/build/Win32/Release")
endif()

if(NOT blackbone_lib64_path)
    # blackbone 64位静态库路径，必须设置！！！
    set(blackbone_lib64_path "E:/git_pro/Blackbone/build/X64/Release")
endif()

# 检查头文件是否设置正确
if(EXISTS ${blackbone_include_path}/BlackBone/Process/Process.h)
    message(STATUS "${blackbone_include_path}/BlackBone/Process/Process.h finded" )
else()
    message(FATAL_ERROR "${blackbone_include_path}/BlackBone/Process/Process.h not finded" )
endif()

# 检查库是否设置正确
if(EXISTS ${blackbone_lib32_path}/blackbone.lib)
    message(STATUS "32 blackbone lib find" )
else()
message(FATAL_ERROR "${blackbone_lib32_path}/blackbone.lib NOT find" )
endif()
if(EXISTS ${blackbone_lib64_path}/blackbone.lib)
    message(STATUS "64 blackbone lib find" )
else()
message(FATAL_ERROR "${blackbone_lib64_path}/blackbone.lib NOT find" )
endif()


project ("op")
add_compile_options("$<$<C_COMPILER_ID:MSVC>:/utf-8>")
add_compile_options("$<$<CXX_COMPILER_ID:MSVC>:/utf-8>")

set(CMAKE_CXX_STANDARD 17)

if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
    MESSAGE(STATUS "----------------Now is MSVC EHA----------------")
    set(CMAKE_CXX_FLAGS_RELEASE "/MT /O2 /EHa")
    set(CMAKE_C_FLAGS_RELEASE "/MT  /O2 /EHa")
    set(CMAKE_CXX_FLAGS_DEBUG "/MT /Zi /EHa")
    set(CMAKE_C_FLAGS_DEBUG "/MT /Zi /EHa")
    set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "/MT /O2  /EHa")
    set(CMAKE_C_FLAGS_RELWITHDEBINFO "/MT /O2 /EHa")
endif()


IF(CMAKE_SIZEOF_VOID_P EQUAL 8)
    link_directories(
        ${CMAKE_SOURCE_DIR}/3rd_party/lib/x64
        ${blackbone_lib64_path}
    )
    set(op_com op_x64)
	MESSAGE(STATUS "----------------Platform x64----------------")
ELSE()
    link_directories(
        ${CMAKE_SOURCE_DIR}/3rd_party/lib/x86
        ${blackbone_lib32_path}
        )
    set(op_com op_x86)
	MESSAGE(STATUS "----------------Platform x86----------------")
ENDIF()

include_directories( 
  ./
  ${CMAKE_SOURCE_DIR}/3rd_party/include/
  ${CMAKE_SOURCE_DIR}/3rd_party/include/3rd_party
  ${blackbone_include_path}
  ${blackbone_include_path}/3rd_party
) 



# 包含子项目。
## libop主项目
add_subdirectory ("libop")
## python测试项目
add_subdirectory("python")
## lua测试项目
add_subdirectory("lua")
## tools项目
add_subdirectory("tools")
## 测试项目
add_subdirectory("tests")
message(STATUS ${build_swig_py})
## swig 项目
if(build_swig_py)
    message(STATUS "build swig is on")
    0add_subdirectory("swig")
endif()
message(STATUS ${build_qttool})
## qttool项目
if(build_qttool)
    message(STATUS "build qt tool is on")
    add_subdirectory("qttool")
endif()

