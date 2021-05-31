cmake_minimum_required(VERSION 3.0)

ADD_DEFINITIONS(-D _UNICODE)
ADD_DEFINITIONS(-D UNICODE)

ADD_DEFINITIONS(-D _CMAKE_BUILD)

SET(SRC_FILES "op_wrap.cxx")

add_library(opEx SHARED ${SRC_FILES})

target_include_directories(opEx PUBLIC
"${PROJECT_BINARY_DIR}"
"${CMAKE_CURRENT_SOURCE_DIR}"
"D:\\py36\\include"
)

link_directories(opEx PUBLIC "D:\\py36\\libs")
#find_library(OP_LIB op ../bin/Release NO_DEFAULT_PATH)
set_target_properties(opEx PROPERTIES PREFIX "_")
set_target_properties(opEx PROPERTIES SUFFIX ".pyd")


target_link_libraries(opEx op_x86)
target_link_libraries(opEx python36.lib)

IF(CMAKE_CL_64)
  install(TARGETS opEx RUNTIME DESTINATION "${PROJECT_SOURCE_DIR}/bin/x64")
  install(TARGETS opEx ARCHIVE DESTINATION "${PROJECT_SOURCE_DIR}/lib/x64")
  install(FILES opEx.py DESTINATION "${PROJECT_SOURCE_DIR}/bin/x64")
ELSE(CMAKE_CL_64)
  install(TARGETS opEx RUNTIME DESTINATION "${PROJECT_SOURCE_DIR}/bin/x86")
  install(TARGETS opEx ARCHIVE DESTINATION "${PROJECT_SOURCE_DIR}/lib/x86")
  install(FILES opEx.py DESTINATION "${PROJECT_SOURCE_DIR}/bin/x86")
ENDIF(CMAKE_CL_64)
