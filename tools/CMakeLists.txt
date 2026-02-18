cmake_minimum_required(VERSION 3.0)

ADD_DEFINITIONS(-D _UNICODE)
ADD_DEFINITIONS(-D UNICODE)

ADD_DEFINITIONS(-D _CMAKE_BUILD)

#ADD_DEFINITIONS(-D EASYCOM_EXPORTS)

# find_library(minhook_lib libminhook.x86 ./3rd_party/lib/x86 NO_DEFAULT_PATH)

SET(TOOLS_SRC_FILES 
"dllmain.c"
"../libop/com/op_i.c"
"easycom.cpp")


add_library(tools SHARED ${TOOLS_SRC_FILES})

# 使用 vcpkg 提供的 MinHook imported target（若存在），否则退回到直接链接库名
if (TARGET minhook::minhook)
  TARGET_LINK_LIBRARIES(tools minhook::minhook)
else()
  TARGET_LINK_LIBRARIES(tools minhook)
endif()


IF(CMAKE_SIZEOF_VOID_P EQUAL 8)
    install(TARGETS tools RUNTIME DESTINATION "${PROJECT_SOURCE_DIR}/bin/x64")
    install(TARGETS tools ARCHIVE DESTINATION "${PROJECT_SOURCE_DIR}/lib/x64")
ELSE()
    install(TARGETS tools RUNTIME DESTINATION "${PROJECT_SOURCE_DIR}/bin/x86")
    install(TARGETS tools ARCHIVE DESTINATION "${PROJECT_SOURCE_DIR}/lib/x86")
ENDIF()