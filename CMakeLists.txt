# CMakeList.txt: 顶层 CMake 项目文件，在此处执行全局配置
# 并包含子项目。
#
cmake_minimum_required (VERSION 3.0)

# SET(CMAKE_TOOLCHAIN_FILE "E:/git_pro/vcpkg/scripts/buildsystems/vcpkg.cmake")
# SET(VCPKG_TARGET_TRIPLET x86-windows-static)

# set(VCPKG_INSTALL_PREFIX ${CMAKE_CURRENT_SOURCE_DIR})
option(build_swig_py "build swig py,requrie python" OFF)
option(build_qttool "buid qt tool,requrie qt lib" OFF)

set(build_qttool ON)
set(build_swig_py OFF)

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


IF(CMAKE_CL_64)
    link_directories(${CMAKE_SOURCE_DIR}/3rd_party/lib/x64)
    set(op_com op_x64)
	MESSAGE(STATUS "----------------Platform x64----------------")
ELSE(CMAKE_CL_64)
    link_directories(${CMAKE_SOURCE_DIR}/3rd_party/lib/x86)
    set(op_com op_x86)
	MESSAGE(STATUS "----------------Platform x86----------------")
ENDIF(CMAKE_CL_64)

include_directories( 
  ./
  ${CMAKE_SOURCE_DIR}/3rd_party/include/
  ${CMAKE_SOURCE_DIR}/3rd_party/include/3rd_party
  "E:/git_pro/Blackbone/src"
   "E:/git_pro/Blackbone/src/3rd_party"
) 

# 包含子项目。
add_subdirectory ("libop")
add_subdirectory("python")
add_subdirectory("lua")
add_subdirectory("tools")
add_subdirectory("tests")
message(STATUS ${build_swig_py})
if(build_swig_py)
    message(STATUS "build swig is on")
    add_subdirectory("swig")
endif()
message(STATUS ${build_qttool})
if(build_qttool)
    message(STATUS "build qt tool is on")
    add_subdirectory("qttool")
endif()

