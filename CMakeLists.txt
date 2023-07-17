# CMakeList.txt: 顶层 CMake 项目文件，在此处执行全局配置
# 并包含子项目。
#
cmake_minimum_required (VERSION 3.0)

if(NOT $ENV{VCPKG_ROOT} STREQUAL "")
    set(${CMAKE_TOOLCHAIN_FILE} $ENV{VCPKG_ROOT})
endif()

if(NOT $ENV{BLACKBONE_ROOT} STREQUAL "")
    set(blackbone_include_path "$ENV{BLACKBONE_ROOT}/src")
    set(blackbone_lib32_path "$ENV{BLACKBONE_ROOT}/build/Win32/Release")
    set(blackbone_lib64_path "$ENV{BLACKBONE_ROOT}/build/X64/Release")
endif()


option(build_swig_py "build swig py,requrie python" OFF)
option(build_qttool "buid qt tool,requrie qt lib" OFF)




# 检查头文件是否设置正确
if(EXISTS ${blackbone_include_path}/BlackBone/Process/Process.h)
    message(STATUS "${blackbone_include_path}/BlackBone/Process/Process.h finded" )
else()
    message(FATAL_ERROR "${blackbone_include_path}/BlackBone/Process/Process.h not finded" )
endif()

# 检查库是否设置正确
if(EXISTS ${blackbone_lib32_path}/BlackBone.lib)
    message(STATUS "32 blackbone lib find" )
else()
message(FATAL_ERROR "${blackbone_lib32_path}/BlackBone.lib NOT find" )
endif()
if(EXISTS ${blackbone_lib64_path}/BlackBone.lib)
    message(STATUS "64 BlackBone lib find" )
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
## swig 项目
if(build_swig_py)
    add_subdirectory("swig")
endif()

## qttool项目
if(build_qttool)
    # 只编译32位
    if(${CMAKE_SIZEOF_VOID_P}$ EQUAL 4)
        add_subdirectory("qttool")
    endif()
endif()

