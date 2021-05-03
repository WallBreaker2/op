cmake_minimum_required(VERSION 3.0)

ADD_DEFINITIONS(-D _UNICODE)
ADD_DEFINITIONS(-D UNICODE)

ADD_DEFINITIONS(-D _CMAKE_BUILD)

SET(SRC_FILES "main.cpp")



add_executable(tests ${SRC_FILES})


target_link_libraries(tests libop)

install(TARGETS tests  DESTINATION "${PROJECT_SOURCE_DIR}/bin")