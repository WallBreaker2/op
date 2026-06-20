cmake_minimum_required(VERSION 3.10)

# find_library(minhook_lib libminhook.x86 ./3rd_party/lib/x86 NO_DEFAULT_PATH)

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
  "${PROJECT_SOURCE_DIR}/3rd_party/include"
  "${PROJECT_SOURCE_DIR}/3rd_party/include/3rd_party"
)

# MinHook 使用 vcpkg static triplet 的静态库，避免运行时依赖 minhook.x*.dll。
set(OP_TOOLS_MINHOOK_LINKED FALSE)
set(OP_VCPKG_INSTALLED_DIR "")
if (DEFINED VCPKG_INSTALLED_DIR)
  set(OP_VCPKG_INSTALLED_DIR "${VCPKG_INSTALLED_DIR}")
elseif(DEFINED _VCPKG_INSTALLED_DIR)
  set(OP_VCPKG_INSTALLED_DIR "${_VCPKG_INSTALLED_DIR}")
elseif(DEFINED ENV{VCPKG_ROOT})
  file(TO_CMAKE_PATH "$ENV{VCPKG_ROOT}/installed" OP_VCPKG_INSTALLED_DIR)
endif()

if (OP_VCPKG_INSTALLED_DIR)
  if (CMAKE_SIZEOF_VOID_P EQUAL 8)
    set(OP_TOOLS_MINHOOK_STATIC_TRIPLET "x64-windows-static")
    set(OP_TOOLS_MINHOOK_LIB_RELEASE_NAME "minhook.x64.lib")
    set(OP_TOOLS_MINHOOK_LIB_DEBUG_NAME "minhook.x64d.lib")
  else()
    set(OP_TOOLS_MINHOOK_STATIC_TRIPLET "x86-windows-static")
    set(OP_TOOLS_MINHOOK_LIB_RELEASE_NAME "minhook.x32.lib")
    set(OP_TOOLS_MINHOOK_LIB_DEBUG_NAME "minhook.x32d.lib")
  endif()

  set(OP_TOOLS_MINHOOK_LIB_RELEASE "${OP_VCPKG_INSTALLED_DIR}/${OP_TOOLS_MINHOOK_STATIC_TRIPLET}/lib/${OP_TOOLS_MINHOOK_LIB_RELEASE_NAME}")
  set(OP_TOOLS_MINHOOK_LIB_DEBUG "${OP_VCPKG_INSTALLED_DIR}/${OP_TOOLS_MINHOOK_STATIC_TRIPLET}/debug/lib/${OP_TOOLS_MINHOOK_LIB_DEBUG_NAME}")

  if (EXISTS "${OP_TOOLS_MINHOOK_LIB_RELEASE}")
    TARGET_LINK_LIBRARIES(tools "${OP_TOOLS_MINHOOK_LIB_RELEASE}")
    set(OP_TOOLS_MINHOOK_LINKED TRUE)
    message(STATUS "Using static MinHook library: ${OP_TOOLS_MINHOOK_LIB_RELEASE}")
  endif()
endif()

if (NOT OP_TOOLS_MINHOOK_LINKED)
  if (TARGET minhook::minhook)
    TARGET_LINK_LIBRARIES(tools minhook::minhook)
  else()
    TARGET_LINK_LIBRARIES(tools minhook)
  endif()
endif()


IF(CMAKE_SIZEOF_VOID_P EQUAL 8)
    install(TARGETS tools RUNTIME DESTINATION "${PROJECT_SOURCE_DIR}/bin/x64")
    install(TARGETS tools ARCHIVE DESTINATION "${PROJECT_SOURCE_DIR}/lib/x64")
ELSE()
    install(TARGETS tools RUNTIME DESTINATION "${PROJECT_SOURCE_DIR}/bin/x86")
    install(TARGETS tools ARCHIVE DESTINATION "${PROJECT_SOURCE_DIR}/lib/x86")
ENDIF()
