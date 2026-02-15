cmake_minimum_required(VERSION 3.10)

ADD_DEFINITIONS(-D _UNICODE)
ADD_DEFINITIONS(-D UNICODE)
ADD_DEFINITIONS(-D _CMAKE_BUILD)

# Find Google Test installed via vcpkg
find_package(GTest CONFIG REQUIRED)

set(SRC_FILES "main.cpp" "../libop/com/op_i.c")

include_directories( 
  ./
  ../libop/3rd_party/include/3rd_party
) 

add_executable(op_test ${SRC_FILES})

# Only link GTest::gtest (NOT gtest_main) because we define our own main()
target_link_libraries(op_test ${op_com} GTest::gtest)

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