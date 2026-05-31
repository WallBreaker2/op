cmake_minimum_required(VERSION 3.10)

ADD_DEFINITIONS(-D _UNICODE)
ADD_DEFINITIONS(-D UNICODE)
ADD_DEFINITIONS(-D _CMAKE_BUILD)

# Prefer static-triplet GoogleTest to match /MT runtime and avoid gtest.dll startup crashes.
if(CMAKE_SIZEOF_VOID_P EQUAL 8)
  set(OP_GTEST_STATIC_TRIPLET "x64-windows-static")
else()
  set(OP_GTEST_STATIC_TRIPLET "x86-windows-static")
endif()

set(OP_GTEST_STATIC_OK FALSE)
set(OP_GTEST_LINK_LIBS "")

if(DEFINED ENV{VCPKG_ROOT})
  file(TO_CMAKE_PATH "$ENV{VCPKG_ROOT}" OP_VCPKG_ROOT)
  set(OP_GTEST_STATIC_ROOT "${OP_VCPKG_ROOT}/installed/${OP_GTEST_STATIC_TRIPLET}")
  set(OP_GTEST_STATIC_INCLUDE "${OP_GTEST_STATIC_ROOT}/include")
  set(OP_GTEST_STATIC_LIB_RELEASE "${OP_GTEST_STATIC_ROOT}/lib/gtest.lib")

  if(EXISTS "${OP_GTEST_STATIC_INCLUDE}/gtest/gtest.h"
     AND EXISTS "${OP_GTEST_STATIC_LIB_RELEASE}")
    include_directories("${OP_GTEST_STATIC_INCLUDE}")
    set(OP_GTEST_LINK_LIBS "${OP_GTEST_STATIC_LIB_RELEASE}")
    set(OP_GTEST_STATIC_OK TRUE)
    message(STATUS "Using static GoogleTest triplet: ${OP_GTEST_STATIC_TRIPLET}")
  endif()
endif()

if(NOT OP_GTEST_STATIC_OK)
  find_package(GTest CONFIG REQUIRED)
  set(OP_GTEST_LINK_LIBS GTest::gtest)
endif()

set(SRC_FILES
  "main.cpp"
  "test_support.cpp"
  "handle_compat_test.cpp"
  "core_algorithm_winapi_test.cpp"
  "mouse_keyboard_test.cpp"
  "image_color_test.cpp"
  "wgc_test.cpp"
  "ocr_test.cpp"
  "opencv_test.cpp"
  "utils_test.cpp"
  "../libop/com/op_i.c"
)

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/keypad/edit_control_keypad_test.cpp")
  list(APPEND SRC_FILES "keypad/edit_control_keypad_test.cpp")
endif()

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/keypad/keypad_message_test.cpp")
  list(APPEND SRC_FILES "keypad/keypad_message_test.cpp")
endif()

include_directories( 
  ./
  ../libop/3rd_party/include/3rd_party
) 

add_executable(op_test ${SRC_FILES})

# Only link GTest::gtest (NOT gtest_main) because we define our own main()
target_link_libraries(op_test ${op_com} ${OP_GTEST_LINK_LIBS})

# Register as a single CTest entry; Google Test handles sub-test filtering internally
# (gtest_discover_tests is not used because it runs the exe at build time,
#  which fails when dependent DLLs like op_x64.dll are not on PATH)
add_test(NAME op_test COMMAND op_test)
set_tests_properties(op_test
    PROPERTIES ENVIRONMENT "PATH=$<TARGET_FILE_DIR:${op_com}>;$ENV{PATH}")

# Install rules
IF(CMAKE_CL_64)
  install(TARGETS op_test DESTINATION "${PROJECT_SOURCE_DIR}/bin/x64")
ELSE()
  install(TARGETS op_test DESTINATION "${PROJECT_SOURCE_DIR}/bin/x86")
ENDIF()

set(OP_DEBUG_SOURCES
  "debug_runner/main_test.cpp"
)

add_executable(op_debug ${OP_DEBUG_SOURCES})
target_link_libraries(op_debug ${op_com})

set_target_properties(op_debug PROPERTIES
  VS_DEBUGGER_WORKING_DIRECTORY "$<TARGET_FILE_DIR:op_debug>"
  VS_DEBUGGER_COMMAND_ARGUMENTS "smoke"
)

IF(CMAKE_CL_64)
  install(TARGETS op_debug DESTINATION "${PROJECT_SOURCE_DIR}/bin/x64")
ELSE()
  install(TARGETS op_debug DESTINATION "${PROJECT_SOURCE_DIR}/bin/x86")
ENDIF()
