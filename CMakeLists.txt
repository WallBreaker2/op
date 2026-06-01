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

set(_op_target_is_64bit FALSE)
if(CMAKE_GENERATOR_PLATFORM STREQUAL "x64" OR CMAKE_VS_PLATFORM_NAME STREQUAL "x64")
    set(_op_target_is_64bit TRUE)
elseif(DEFINED CMAKE_SIZEOF_VOID_P AND CMAKE_SIZEOF_VOID_P EQUAL 8)
    set(_op_target_is_64bit TRUE)
endif()

if(build_swig_py)
    if(_op_target_is_64bit)
        set(_op_python_home "$ENV{PYTHON64_ROOT}")
    else()
        set(_op_python_home "$ENV{PYTHON32_ROOT}")
    endif()
    set(_op_python_bitness "")

    if(_op_python_home STREQUAL "" AND DEFINED ENV{PYTHONHOME})
        set(_op_python_home "$ENV{PYTHONHOME}")
    endif()

    if(_op_python_home STREQUAL "")
        find_package(Python3 COMPONENTS Interpreter QUIET)
        if(Python3_Interpreter_FOUND)
            get_filename_component(_op_python_home "${Python3_EXECUTABLE}" DIRECTORY)
            execute_process(
                COMMAND "${Python3_EXECUTABLE}" -c "import struct; print(struct.calcsize('P') * 8)"
                OUTPUT_VARIABLE _op_python_bitness
                OUTPUT_STRIP_TRAILING_WHITESPACE
                ERROR_QUIET
            )
        endif()
    endif()

    if(NOT _op_python_home STREQUAL "")
        file(TO_CMAKE_PATH "${_op_python_home}" _op_python_home)
        if(_op_target_is_64bit)
            if(NOT EXISTS "${_op_python_home}/python312.dll"
               AND NOT EXISTS "${_op_python_home}/python311.dll"
               AND NOT EXISTS "${_op_python_home}/python310.dll"
               AND NOT EXISTS "${_op_python_home}/python39.dll")
                message(STATUS "Disable swig python build: no supported 64-bit Python runtime found under ${_op_python_home}")
                set(build_swig_py OFF)
            elseif(NOT EXISTS "${_op_python_home}/libs/python312.lib"
                   AND NOT EXISTS "${_op_python_home}/libs/python311.lib"
                   AND NOT EXISTS "${_op_python_home}/libs/python310.lib"
                   AND NOT EXISTS "${_op_python_home}/libs/python39.lib")
                message(STATUS "Disable swig python build: no matching 64-bit Python import library found under ${_op_python_home}/libs")
                set(build_swig_py OFF)
            endif()
        else()
            if(_op_python_bitness STREQUAL "64")
                message(STATUS "Disable swig python build: detected Python interpreter is 64-bit but current target is x86")
                set(build_swig_py OFF)
            elseif(NOT EXISTS "${_op_python_home}/python312.dll"
                   AND NOT EXISTS "${_op_python_home}/python311.dll"
                   AND NOT EXISTS "${_op_python_home}/python310.dll"
                   AND NOT EXISTS "${_op_python_home}/python39.dll")
                message(STATUS "Disable swig python build: no supported 32-bit Python runtime found under ${_op_python_home}")
                set(build_swig_py OFF)
            elseif(EXISTS "${_op_python_home}/python312.dll"
                   OR EXISTS "${_op_python_home}/python311.dll"
                   OR EXISTS "${_op_python_home}/python310.dll"
                   OR EXISTS "${_op_python_home}/python39.dll")
                if(NOT EXISTS "${_op_python_home}/libs/python312.lib"
                   AND NOT EXISTS "${_op_python_home}/libs/python311.lib"
                   AND NOT EXISTS "${_op_python_home}/libs/python310.lib"
                   AND NOT EXISTS "${_op_python_home}/libs/python39.lib")
                    message(STATUS "Disable swig python build: no matching 32-bit Python import library found under ${_op_python_home}/libs")
                    set(build_swig_py OFF)
                endif()
            endif()
        endif()
    else()
        message(STATUS "Disable swig python build: Python runtime not found")
        set(build_swig_py OFF)
    endif()
endif()

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

option(enable_wgc "Enable Windows Graphics Capture backend" ON)

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

if(enable_wgc)
    set(enable_wgc ON CACHE BOOL "Enable Windows Graphics Capture backend" FORCE)
    add_compile_definitions(OP_ENABLE_WGC)
    message(STATUS "----------------WGC enabled----------------")
endif()

if(BLACKBONE_LIBRARY STREQUAL "" AND NOT BLACKBONE_ROOT STREQUAL "")
    if(_op_target_is_64bit)
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
            if(_op_target_is_64bit)
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
