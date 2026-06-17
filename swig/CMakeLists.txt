cmake_minimum_required(VERSION 3.10)

ADD_DEFINITIONS(-D _UNICODE)
ADD_DEFINITIONS(-D UNICODE)

ADD_DEFINITIONS(-D _CMAKE_BUILD)

SET(SRC_FILES "op_wrap.cxx")

add_library(pyop SHARED ${SRC_FILES})

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

    if(EXISTS "${PYTHON_HOME}/python313.dll")
        set(PYTHON_LIBRARY_NAME "python313")
    elseif(EXISTS "${PYTHON_HOME}/python312.dll")
        set(PYTHON_LIBRARY_NAME "python312")
    elseif(EXISTS "${PYTHON_HOME}/python311.dll")
        set(PYTHON_LIBRARY_NAME "python311")
    elseif(EXISTS "${PYTHON_HOME}/python310.dll")
        set(PYTHON_LIBRARY_NAME "python310")
    elseif(EXISTS "${PYTHON_HOME}/python39.dll")
        set(PYTHON_LIBRARY_NAME "python39")
    endif()

    if(PYTHON_LIBRARY_NAME STREQUAL "")
        message(FATAL_ERROR "Unable to determine Python import library under ${PYTHON_HOME}")
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
    IF(CMAKE_SIZEOF_VOID_P EQUAL 8)
        install(TARGETS pyop RUNTIME DESTINATION "${PROJECT_SOURCE_DIR}/bin/x64")
        install(TARGETS pyop ARCHIVE DESTINATION "${PROJECT_SOURCE_DIR}/lib/x64")
        install(FILES pyop.py DESTINATION "${PROJECT_SOURCE_DIR}/bin/x64")
    ELSE()
        install(TARGETS pyop RUNTIME DESTINATION "${PROJECT_SOURCE_DIR}/bin/x86")
        install(TARGETS pyop ARCHIVE DESTINATION "${PROJECT_SOURCE_DIR}/lib/x86")
        install(FILES pyop.py DESTINATION "${PROJECT_SOURCE_DIR}/bin/x86")
    ENDIF()
endif()
