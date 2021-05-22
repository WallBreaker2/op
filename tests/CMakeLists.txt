cmake_minimum_required(VERSION 3.0)

ADD_DEFINITIONS(-D _UNICODE)
ADD_DEFINITIONS(-D UNICODE)

ADD_DEFINITIONS(-D _CMAKE_BUILD)

SET(SRC_FILES "main.cpp"
"../com/op_i.c")


include_directories( 
  ./
  ../libop/3rd_party/include/3rd_party
) 


add_executable(tests ${SRC_FILES})


target_link_libraries(tests ${op_com})


IF(CMAKE_CL_64)
  install(TARGETS tests  DESTINATION "${PROJECT_SOURCE_DIR}/bin/x64")
ELSE(CMAKE_CL_64)
  install(TARGETS tests  DESTINATION "${PROJECT_SOURCE_DIR}/bin/x86")
ENDIF(CMAKE_CL_64)