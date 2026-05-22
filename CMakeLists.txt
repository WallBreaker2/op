cmake_minimum_required (VERSION 3.10)

set(BLACKBONE_ROOT "$ENV{BLACKBONE_ROOT}" CACHE PATH "Path to BlackBone repository root")
set(BLACKBONE_INCLUDE_DIR "" CACHE PATH "Path to BlackBone include root (contains BlackBone/Process/Process.h)")
set(BLACKBONE_LIBRARY "" CACHE FILEPATH "Full path to BlackBone.lib")

if(NOT BLACKBONE_ROOT STREQUAL "")
    file(TO_CMAKE_PATH "${BLACKBONE_ROOT}" BLACKBONE_ROOT)
endif()

if(NOT BLACKBONE_INCLUDE_DIR STREQUAL "")
    file(TO_CMAKE_PATH "${BLACKBONE_INCLUDE_DIR}" BLACKBONE_INCLUDE_DIR)
endif()

if(NOT BLACKBONE_LIBRARY STREQUAL "")
    file(TO_CMAKE_PATH "${BLACKBONE_LIBRARY}" BLACKBONE_LIBRARY)
endif()

if(BLACKBONE_INCLUDE_DIR STREQUAL "" AND NOT BLACKBONE_ROOT STREQUAL "")
    if(EXISTS "${BLACKBONE_ROOT}/src/BlackBone/Process/Process.h")
        set(BLACKBONE_INCLUDE_DIR "${BLACKBONE_ROOT}/src")
    elseif(EXISTS "${BLACKBONE_ROOT}/BlackBone/Process/Process.h")
        set(BLACKBONE_INCLUDE_DIR "${BLACKBONE_ROOT}")
    endif()
endif()

if(BLACKBONE_INCLUDE_DIR STREQUAL "")
    message(FATAL_ERROR "BlackBone include path is not set. Please configure BLACKBONE_INCLUDE_DIR or BLACKBONE_ROOT.")
endif()


option(build_swig_py "build swig py,requrie python" ON)

# 检查头文件是否设置正确
if(EXISTS "${BLACKBONE_INCLUDE_DIR}/BlackBone/Process/Process.h")
    message(STATUS "${BLACKBONE_INCLUDE_DIR}/BlackBone/Process/Process.h finded" )
else()
    message(FATAL_ERROR "${BLACKBONE_INCLUDE_DIR}/BlackBone/Process/Process.h not finded" )
endif()



# Integrate vcpkg toolchain if VCPKG_ROOT is set
if(DEFINED ENV{VCPKG_ROOT} AND NOT DEFINED CMAKE_TOOLCHAIN_FILE)
    set(CMAKE_TOOLCHAIN_FILE "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake"
        CACHE STRING "Vcpkg toolchain file")
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
    )
    set(op_com op_x64)
    add_compile_definitions(_AMD64_)
	MESSAGE(STATUS "----------------Platform x64----------------")
ELSE()
    link_directories(
        ${CMAKE_SOURCE_DIR}/3rd_party/lib/x86
        )
    set(op_com op_x86)
    add_compile_definitions(_X86_)
	MESSAGE(STATUS "----------------Platform x86----------------")
ENDIF()

if(BLACKBONE_LIBRARY STREQUAL "" AND NOT BLACKBONE_ROOT STREQUAL "")
    if(CMAKE_SIZEOF_VOID_P EQUAL 8)
        set(_blackbone_candidates
            "${BLACKBONE_ROOT}/build/X64/Release/BlackBone.lib"
            "${BLACKBONE_ROOT}/build/x64/Release/BlackBone.lib"
            "${BLACKBONE_ROOT}/build/X64/BlackBone/Release/BlackBone.lib"
            "${BLACKBONE_ROOT}/build/x64/BlackBone/Release/BlackBone.lib"
            "${BLACKBONE_ROOT}/build/X64/src/BlackBone/Release/BlackBone.lib"
            "${BLACKBONE_ROOT}/build/x64/src/BlackBone/Release/BlackBone.lib"
        )
    else()
        set(_blackbone_candidates
            "${BLACKBONE_ROOT}/build/Win32/Release/BlackBone.lib"
            "${BLACKBONE_ROOT}/build/x86/Release/BlackBone.lib"
            "${BLACKBONE_ROOT}/build/Win32/BlackBone/Release/BlackBone.lib"
            "${BLACKBONE_ROOT}/build/x86/BlackBone/Release/BlackBone.lib"
            "${BLACKBONE_ROOT}/build/Win32/src/BlackBone/Release/BlackBone.lib"
            "${BLACKBONE_ROOT}/build/x86/src/BlackBone/Release/BlackBone.lib"
        )
    endif()

    foreach(_candidate IN LISTS _blackbone_candidates)
        if(EXISTS "${_candidate}")
            set(BLACKBONE_LIBRARY "${_candidate}")
            break()
        endif()
    endforeach()

    if(BLACKBONE_LIBRARY STREQUAL "")
        file(GLOB_RECURSE _blackbone_all_libs
            "${BLACKBONE_ROOT}/build/BlackBone.lib"
            "${BLACKBONE_ROOT}/build/*/BlackBone.lib"
        )

        foreach(_lib IN LISTS _blackbone_all_libs)
            file(TO_CMAKE_PATH "${_lib}" _lib_norm)
            string(TOLOWER "${_lib_norm}" _lib_lower)
            if(CMAKE_SIZEOF_VOID_P EQUAL 8)
                if(_lib_lower MATCHES "/x64/" OR _lib_lower MATCHES "/amd64/")
                    set(BLACKBONE_LIBRARY "${_lib_norm}")
                    break()
                endif()
            else()
                if(_lib_lower MATCHES "/win32/" OR _lib_lower MATCHES "/x86/")
                    set(BLACKBONE_LIBRARY "${_lib_norm}")
                    break()
                endif()
            endif()
        endforeach()

        if(BLACKBONE_LIBRARY STREQUAL "" AND _blackbone_all_libs)
            list(GET _blackbone_all_libs 0 BLACKBONE_LIBRARY)
        endif()
    endif()
endif()

if(BLACKBONE_LIBRARY STREQUAL "")
    message(FATAL_ERROR "BlackBone.lib not found. Please configure BLACKBONE_LIBRARY explicitly.")
endif()

if(NOT EXISTS "${BLACKBONE_LIBRARY}")
    message(FATAL_ERROR "${BLACKBONE_LIBRARY} not finded")
endif()

set(BLACKBONE_INCLUDE_DIR "${BLACKBONE_INCLUDE_DIR}" CACHE PATH "Path to BlackBone include root (contains BlackBone/Process/Process.h)" FORCE)
set(BLACKBONE_LIBRARY "${BLACKBONE_LIBRARY}" CACHE FILEPATH "Full path to BlackBone.lib" FORCE)

message(STATUS "BlackBone include: ${BLACKBONE_INCLUDE_DIR}")
message(STATUS "BlackBone library: ${BLACKBONE_LIBRARY}")

include_directories( 
  ./
  ${CMAKE_SOURCE_DIR}/3rd_party/include/
  ${CMAKE_SOURCE_DIR}/3rd_party/include/3rd_party
  ${BLACKBONE_INCLUDE_DIR}
  ${BLACKBONE_INCLUDE_DIR}/3rd_party
) 

# 包含子项目。
## libop主项目
add_subdirectory ("libop")
## tools项目
add_subdirectory("tools")
## 测试项目
add_subdirectory("tests")
## swig 项目
if(build_swig_py)
    add_subdirectory("swig")
endif()
