cmake_minimum_required (VERSION 3.10)

if(NOT $ENV{BLACKBONE_ROOT} STREQUAL "")
    set(blackbone_include_path "$ENV{BLACKBONE_ROOT}/src")
    set(blackbone_lib32_path "$ENV{BLACKBONE_ROOT}/build/Win32/Release")
    set(blackbone_lib64_path "$ENV{BLACKBONE_ROOT}/build/X64/Release")
endif()


option(build_swig_py "build swig py,requrie python" ON)

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
include(CTest)
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
    add_compile_definitions(_AMD64_)
	MESSAGE(STATUS "----------------Platform x64----------------")
ELSE()
    link_directories(
        ${CMAKE_SOURCE_DIR}/3rd_party/lib/x86
        ${blackbone_lib32_path}
        )
    set(op_com op_x86)
    add_compile_definitions(_X86_)
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
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/libop/com/op_i.h")
    ## tools项目
    add_subdirectory("tools")
    ## 测试项目
    add_subdirectory("tests")
endif()
## swig 项目
if(build_swig_py)
    add_subdirectory("swig")
endif()


