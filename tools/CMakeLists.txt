cmake_minimum_required(VERSION 3.0)

ADD_DEFINITIONS(-D _UNICODE)
ADD_DEFINITIONS(-D UNICODE)

ADD_DEFINITIONS(-D _CMAKE_BUILD)

ADD_DEFINITIONS(-D EASYCOM_EXPORTS)

# find_library(minhook_lib libminhook.x86 ./3rd_party/lib/x86 NO_DEFAULT_PATH)

SET(TOOLS_SRC_FILES 
"dllmain.c"
"../com/op_i.c"
"easycom.cpp")


add_library(tools SHARED ${TOOLS_SRC_FILES})

TARGET_LINK_LIBRARIES(tools ${minhook_lib})

install(TARGETS tools RUNTIME DESTINATION "${PROJECT_SOURCE_DIR}/bin")
install(TARGETS tools ARCHIVE DESTINATION "${PROJECT_SOURCE_DIR}/lib")
