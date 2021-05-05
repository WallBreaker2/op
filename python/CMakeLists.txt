cmake_minimum_required(VERSION 3.0)

ADD_DEFINITIONS(-D _UNICODE)
ADD_DEFINITIONS(-D UNICODE)

ADD_DEFINITIONS(-D _CMAKE_BUILD)

#SET(SRC_FILES "main.cpp")



#find_library(OP_LIB op ../bin/Release NO_DEFAULT_PATH)

#add_executable(test ${SRC_FILES})

#target_link_libraries(test ${OP_LIB})
install(FILES test.py DESTINATION "${PROJECT_SOURCE_DIR}/bin")
