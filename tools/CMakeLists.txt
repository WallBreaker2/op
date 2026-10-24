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

# op_inject64：64 位注入助手（只编 x64 一份）。
#
# 32 位宿主绑原生 64 位目标时，宿主自己注入不了（BlackBone 跨位宽 RPC 选错汇编器，
# 见 doc/defect_fixes.md §72.13），改为 CreateProcess 这个助手，由它完成
# LoadLibraryW + SetDisplayHook/ReleaseDisplayHook 远程调用。
# 32 位构建里不会产出它 —— x86 的产物目录靠 stage 脚本从 x64 输出拷过去。
IF(CMAKE_SIZEOF_VOID_P EQUAL 8)
    add_executable(op_inject64 "inject64/main.cpp")
    target_compile_definitions(op_inject64 PRIVATE _UNICODE UNICODE _CRT_SECURE_NO_WARNINGS)
    set_target_properties(op_inject64 PROPERTIES OUTPUT_NAME "op_inject64")
    install(TARGETS op_inject64 RUNTIME DESTINATION "${PROJECT_SOURCE_DIR}/bin/x64")
    # 32 位宿主目录里也要有它（跨位宽绑定时从 op_x86.dll 旁边启动）
    install(TARGETS op_inject64 RUNTIME DESTINATION "${PROJECT_SOURCE_DIR}/bin/x86")
ENDIF()
