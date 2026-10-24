cmake_minimum_required(VERSION 3.10)

SET(SRC_FILES "op_wrap.cxx")

add_library(pyop SHARED ${SRC_FILES})
target_compile_definitions(pyop PRIVATE _UNICODE UNICODE _CMAKE_BUILD)
if(DEFINED OP_ARCH_DEFINITION AND NOT OP_ARCH_DEFINITION STREQUAL "")
    target_compile_definitions(pyop PRIVATE "${OP_ARCH_DEFINITION}")
endif()

set_target_properties(pyop PROPERTIES PREFIX "_")
set_target_properties(pyop PROPERTIES SUFFIX ".pyd")

if(OP_PYTHON_WHEEL)
    find_package(Python3 COMPONENTS Interpreter Development REQUIRED)
    message(STATUS "Wheel build Python: ${Python3_EXECUTABLE} (${Python3_VERSION})")
    target_include_directories(pyop PRIVATE
        "${PROJECT_BINARY_DIR}"
        "${CMAKE_CURRENT_SOURCE_DIR}"
        ${Python3_INCLUDE_DIRS}
    )
    target_link_libraries(pyop PRIVATE ${op_com} Python3::Module)
else()
    set(PYTHON_HOME "")
    set(PYTHON_LIBRARY_NAME "")

    if(CMAKE_SIZEOF_VOID_P EQUAL 8)
        set(PYTHON_HOME "$ENV{PYTHON64_ROOT}")
    else()
        set(PYTHON_HOME "$ENV{PYTHON32_ROOT}")
    endif()

    if(PYTHON_HOME STREQUAL "")
        find_package(Python3 COMPONENTS Interpreter REQUIRED)
        get_filename_component(PYTHON_HOME "${Python3_EXECUTABLE}" DIRECTORY)
    endif()

    if(PYTHON_HOME STREQUAL "")
        message(FATAL_ERROR "Python home not found. Please configure PYTHON64_ROOT/PYTHON32_ROOT or install Python 3.")
    endif()

    # 版本无关：取 python3X.dll 与 libs/python3X.lib 成对存在的那个版本。
    # 原来这里是 3.9~3.13 的硬编码链，新版本 Python 会直接落到下面的 FATAL_ERROR。
    file(GLOB _pyop_python_dlls RELATIVE "${PYTHON_HOME}" "${PYTHON_HOME}/python3*.dll")
    foreach(_pyop_dll IN LISTS _pyop_python_dlls)
        if(NOT _pyop_dll MATCHES "^python[0-9]+\\.dll$")
            continue()
        endif()
        string(REGEX REPLACE "\\.dll$" "" _pyop_stem "${_pyop_dll}")
        if(EXISTS "${PYTHON_HOME}/libs/${_pyop_stem}.lib")
            set(PYTHON_LIBRARY_NAME "${_pyop_stem}")
        endif()
    endforeach()

    if(PYTHON_LIBRARY_NAME STREQUAL "")
        message(FATAL_ERROR "Unable to determine Python import library under ${PYTHON_HOME} (looked for python3X.dll + libs/python3X.lib)")
    endif()

    target_include_directories(pyop PUBLIC
        "${PROJECT_BINARY_DIR}"
        "${CMAKE_CURRENT_SOURCE_DIR}"
        "${PYTHON_HOME}\\include"
    )
    message(STATUS "Python libs: ${PYTHON_HOME}\\libs")
    target_link_directories(pyop PUBLIC "${PYTHON_HOME}\\libs")
    target_link_libraries(pyop ${op_com})
    target_link_libraries(pyop ${PYTHON_LIBRARY_NAME})
endif()

if(OP_PYTHON_WHEEL)
    install(TARGETS pyop RUNTIME DESTINATION pyop)
    install(TARGETS ${op_com} RUNTIME DESTINATION pyop)
    install(TARGETS tools RUNTIME DESTINATION pyop)
else()
    # python/pyop/_binding.py 是入库的权威副本；swig/pyop.py 只是 swig 的中间产物（不入库）。
    # 这里把它按传统布局改名安装成 pyop.py。
    set(OP_PYOP_WRAPPER "${PROJECT_SOURCE_DIR}/python/pyop/_binding.py")
    IF(CMAKE_SIZEOF_VOID_P EQUAL 8)
        install(TARGETS pyop RUNTIME DESTINATION "${PROJECT_SOURCE_DIR}/bin/x64")
        install(FILES "${OP_PYOP_WRAPPER}" DESTINATION "${PROJECT_SOURCE_DIR}/bin/x64" RENAME pyop.py)
    ELSE()
        install(TARGETS pyop RUNTIME DESTINATION "${PROJECT_SOURCE_DIR}/bin/x86")
        install(FILES "${OP_PYOP_WRAPPER}" DESTINATION "${PROJECT_SOURCE_DIR}/bin/x86" RENAME pyop.py)
    ENDIF()
endif()
