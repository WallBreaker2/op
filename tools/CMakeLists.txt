cmake_minimum_required(VERSION 3.10)

SET(TOOLS_SRC_FILES 
"dllmain.c"
"${OP_COM_I_C}"
"easycom.cpp")
set_source_files_properties("${OP_COM_I_C}" PROPERTIES GENERATED TRUE)


add_library(tools SHARED ${TOOLS_SRC_FILES})
target_compile_definitions(tools PRIVATE _UNICODE UNICODE _CMAKE_BUILD)
if(DEFINED OP_ARCH_DEFINITION AND NOT OP_ARCH_DEFINITION STREQUAL "")
  target_compile_definitions(tools PRIVATE "${OP_ARCH_DEFINITION}")
endif()
target_include_directories(tools PRIVATE
  "${CMAKE_CURRENT_SOURCE_DIR}"
  "${PROJECT_SOURCE_DIR}"
  "${OP_COM_GENERATED_DIR}"
)

# MinHook 通过 vcpkg 提供的 imported target 链接,头文件与库均由 minhook::minhook 提供。
find_package(minhook CONFIG REQUIRED)
target_link_libraries(tools PRIVATE minhook::minhook)


IF(CMAKE_SIZEOF_VOID_P EQUAL 8)
    install(TARGETS tools RUNTIME DESTINATION "${PROJECT_SOURCE_DIR}/bin/x64")
ELSE()
    install(TARGETS tools RUNTIME DESTINATION "${PROJECT_SOURCE_DIR}/bin/x86")
ENDIF()
