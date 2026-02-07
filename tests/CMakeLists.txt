cmake_minimum_required(VERSION 3.10)

ADD_DEFINITIONS(-D _UNICODE)
ADD_DEFINITIONS(-D UNICODE)
ADD_DEFINITIONS(-D _CMAKE_BUILD)

set(SRC_FILES "main.cpp" "../libop/com/op_i.c")

include_directories( 
  ./
  ../libop/3rd_party/include/3rd_party
) 

add_executable(op_test ${SRC_FILES})
target_link_libraries(op_test ${op_com})

# Register modular tests with CTest
add_test(NAME test_core COMMAND op_test core)
add_test(NAME test_algorithm COMMAND op_test algorithm)
add_test(NAME test_winapi COMMAND op_test winapi)
add_test(NAME test_mouse_key COMMAND op_test mouse_key)
add_test(NAME test_image_color COMMAND op_test image_color)
add_test(NAME test_ocr COMMAND op_test ocr)
add_test(NAME test_utils COMMAND op_test utils)

# Set environment path so the tests can find the DLL
set_tests_properties(test_core test_algorithm test_winapi test_mouse_key test_image_color test_ocr test_utils
    PROPERTIES ENVIRONMENT "PATH=$<TARGET_FILE_DIR:${op_com}>;$ENV{PATH}")

# Install rules
IF(CMAKE_CL_64)
  install(TARGETS op_test DESTINATION "${PROJECT_SOURCE_DIR}/bin/x64")
ELSE()
  install(TARGETS op_test DESTINATION "${PROJECT_SOURCE_DIR}/bin/x86")
ENDIF()