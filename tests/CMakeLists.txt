cmake_minimum_required(VERSION 3.0)

ADD_DEFINITIONS(-D _UNICODE)
ADD_DEFINITIONS(-D UNICODE)

ADD_DEFINITIONS(-D _CMAKE_BUILD)

find_package(Tesseract)
include_directories(${Tesseract_INCLUDE_DIRS})
#include_directories("E:/git_pro/vcpkg/installed/x86-windows/include")

SET(SRC_FILES "main.cpp"
"../libop/com/op_i.c")


include_directories( 
  ./
  ../libop/3rd_party/include/3rd_party
) 


add_executable(tests ${SRC_FILES}  )

target_link_libraries(tests ${op_com})
target_link_libraries(tests Tesseract::libtesseract)
IF(CMAKE_CL_64)
  install(TARGETS tests  DESTINATION "${PROJECT_SOURCE_DIR}/bin/x64")
ELSE(CMAKE_CL_64)
  install(TARGETS tests  DESTINATION "${PROJECT_SOURCE_DIR}/bin/x86")
ENDIF(CMAKE_CL_64)