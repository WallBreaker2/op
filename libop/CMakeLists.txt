cmake_minimum_required(VERSION 3.10)

set(OP_HAS_OPENCV FALSE)
set(OP_OPENCV_INCLUDE_DIR "")
set(OP_OPENCV_LIB_DIR "")
set(OP_OPENCV_LINK_LIBS "")
if(NOT DEFINED OPENCV_LIB_SUFFIX)
  set(OPENCV_LIB_SUFFIX "500")
endif()

function(_op_find_static_lib _op_out_var _op_lib_dir _op_lib_name)
  set(_op_candidates
    "${_op_lib_dir}/${_op_lib_name}.lib"
    "${_op_lib_dir}/lib${_op_lib_name}.a"
    "${_op_lib_dir}/${_op_lib_name}.a"
  )
  foreach(_op_candidate IN LISTS _op_candidates)
    if(EXISTS "${_op_candidate}")
      set(${_op_out_var} "${_op_candidate}" PARENT_SCOPE)
      return()
    endif()
  endforeach()
  set(${_op_out_var} "" PARENT_SCOPE)
endfunction()

set(_op_opencv_root "")
if(DEFINED OPENCV_ROOT)
  set(_op_opencv_root "${OPENCV_ROOT}")
elseif(DEFINED ENV{OPENCV_ROOT})
  set(_op_opencv_root "$ENV{OPENCV_ROOT}")
endif()

if(NOT _op_opencv_root STREQUAL "")
  file(TO_CMAKE_PATH "${_op_opencv_root}" _op_opencv_root)
  set(OP_OPENCV_INCLUDE_DIR "${_op_opencv_root}/include")
  if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    set(_op_opencv_arch_dir "x64")
  else()
    set(_op_opencv_arch_dir "x86")
  endif()
  set(_op_opencv_lib_dir_candidates
    "${_op_opencv_root}/${_op_opencv_arch_dir}/vc18/staticlib"
    "${_op_opencv_root}/${_op_opencv_arch_dir}/vc17/staticlib"
    "${_op_opencv_root}/${_op_opencv_arch_dir}/vc16/staticlib"
  )
  foreach(_op_opencv_lib_dir_candidate IN LISTS _op_opencv_lib_dir_candidates)
    if(EXISTS "${_op_opencv_lib_dir_candidate}")
      set(OP_OPENCV_LIB_DIR "${_op_opencv_lib_dir_candidate}")
      break()
    endif()
  endforeach()
  if(OP_OPENCV_LIB_DIR STREQUAL "")
    list(GET _op_opencv_lib_dir_candidates 0 OP_OPENCV_LIB_DIR)
  endif()

  set(_op_required_lib_names
    "opencv_calib${OPENCV_LIB_SUFFIX}"
    "opencv_objdetect${OPENCV_LIB_SUFFIX}"
    "opencv_stereo${OPENCV_LIB_SUFFIX}"
    "opencv_features${OPENCV_LIB_SUFFIX}"
    "opencv_imgcodecs${OPENCV_LIB_SUFFIX}"
    "opencv_imgproc${OPENCV_LIB_SUFFIX}"
    "opencv_geometry${OPENCV_LIB_SUFFIX}"
    "opencv_flann${OPENCV_LIB_SUFFIX}"
    "opencv_core${OPENCV_LIB_SUFFIX}"
    "libclapack"
    "libjpeg-turbo"
    "libpng"
    "zlib"
  )

  set(_op_expected_libs "")
  set(_op_all_libs_exist TRUE)
  foreach(_op_lib_name IN LISTS _op_required_lib_names)
    _op_find_static_lib(_op_lib_path "${OP_OPENCV_LIB_DIR}" "${_op_lib_name}")
    if(_op_lib_path STREQUAL "")
      set(_op_all_libs_exist FALSE)
      message(WARNING "OpenCV library not found: ${_op_lib_name} (.lib/.a) in ${OP_OPENCV_LIB_DIR}")
    else()
      list(APPEND _op_expected_libs "${_op_lib_path}")
    endif()
  endforeach()

  if(EXISTS "${OP_OPENCV_INCLUDE_DIR}/opencv2/core.hpp" AND _op_all_libs_exist)
    set(OP_HAS_OPENCV TRUE)
    set(OP_OPENCV_LINK_LIBS ${_op_expected_libs})
    message(STATUS "OpenCV enabled via manual static libraries: ${OP_OPENCV_LIB_DIR}")
  else()
    message(WARNING "OPENCV_ROOT is set but required OpenCV headers or libraries were not found. Continuing without OpenCV support.")
  endif()
endif()

set(OP_CAPTURE_SOURCES
  "binding/BindingSession.cpp"
  "capture/FrameInfo.cpp"
  "capture/ICaptureBackend.cpp"
  "capture/sources/MemoryImageSource.cpp"
  "capture/backends/GdiCapture.cpp"
  "capture/backends/DxgiCapture.cpp"
  "capture/backends/HookCapture.cpp"
)
if(enable_wgc)
  list(APPEND OP_CAPTURE_SOURCES "capture/backends/WgcCapture.cpp")
endif()

set(OP_INPUT_SOURCES
  "input/mouse/WinMouse.cpp"
  "input/mouse/DxMouse.cpp"
  "input/mouse/CursorShape.cpp"
  "input/keyboard/KeyboardBackend.cpp"
  "input/keyboard/WinKeyboard.cpp"
  "input/keyboard/DxKeyboard.cpp"
)

set(OP_HOOK_SOURCES
  "hook/ApiResolver.cpp"
  "hook/DisplayHook.cpp"
  "hook/InputHook.cpp"
  "hook/InputHookClient.cpp"
  "hook/HookExport.cpp"
  "hook/Dx12Hook.cpp"
  "hook/MinHookRuntime.cpp"
)

set(OP_KIERO2_SOURCES
  "../3rd_party/kiero2/kiero_intern.cpp"
  "../3rd_party/kiero2/kiero_d3d9.cpp"
  "../3rd_party/kiero2/kiero_d3d10.cpp"
  "../3rd_party/kiero2/kiero_d3d11.cpp"
  "../3rd_party/kiero2/kiero_d3d12.cpp"
  "../3rd_party/kiero2/kiero_opengl.cpp"
)

set(OP_RUNTIME_SOURCES
  "runtime/AutomationModes.cpp"
  "runtime/RuntimeUtils.cpp"
  "runtime/RuntimeEnvironment.cpp"
  "runtime/WindowsVersion.cpp"
)

set(OP_WINDOW_SOURCES
  "window/DllInjector.cpp"
  "window/WindowProcess.cpp"
  "window/WindowService.cpp"
  "window/WindowState.cpp"
  "window/WindowText.cpp"
  "window/WindowLayout.cpp"
)

set(OP_IMAGE_SOURCES
  "image/ImageSearchAlgorithms.cpp"
  "image/ImageSearchService.cpp"
)

set(OP_NETWORK_SOURCES
  "network/HttpClient.cpp"
)

set(OP_OCR_SOURCES
  "ocr/OcrService.cpp"
)

set(OP_YOLO_SOURCES
  "yolo/YoloDetector.cpp"
)

set(OP_MEMORY_SOURCES
  "memory/DiaGuids.cpp"
  "memory/ProcessMemory.cpp"
)

set(OP_IPC_SOURCES
  "ipc/Pipe.cpp"
)

set(OP_CLIENT_SOURCES
  "client/ClientContext.cpp"
  "client/ClientRuntime.cpp"
  "client/ClientProcess.cpp"
  "client/ClientClipboard.cpp"
  "client/ClientWindow.cpp"
  "client/ClientInput.cpp"
  "client/ClientAlgorithm.cpp"
  "client/ClientImage.cpp"
  "client/ClientOpenCv.cpp"
  "client/ClientOcr.cpp"
  "client/ClientYolo.cpp"
  "client/ClientMemory.cpp"
)

set(OP_OPENCV_SOURCES "")
if(OP_HAS_OPENCV AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/opencv/TemplateMatcher.cpp")
  set(OP_OPENCV_SOURCES
    "opencv/OpenCvBridge.cpp"
    "opencv/OpenCvHelpers.cpp"
    "opencv/TemplateMatcher.cpp"
  )
endif()

set(OP_LIBOP_SOURCES
  ${OP_CLIENT_SOURCES}
  ${OP_CAPTURE_SOURCES}
  ${OP_INPUT_SOURCES}
  ${OP_HOOK_SOURCES}
  ${OP_RUNTIME_SOURCES}
  ${OP_WINDOW_SOURCES}
  ${OP_IMAGE_SOURCES}
  ${OP_NETWORK_SOURCES}
  ${OP_OCR_SOURCES}
  ${OP_YOLO_SOURCES}
  ${OP_MEMORY_SOURCES}
  ${OP_IPC_SOURCES}
  ${OP_OPENCV_SOURCES}
  ${OP_KIERO2_SOURCES}
  "libop.cpp"
)

set_source_files_properties("../3rd_party/kiero2/kiero_d3d9.cpp" PROPERTIES COMPILE_DEFINITIONS "KIERO_IMPL_FIRST_SLOT=0")
set_source_files_properties("../3rd_party/kiero2/kiero_d3d10.cpp" PROPERTIES COMPILE_DEFINITIONS "KIERO_IMPL_FIRST_SLOT=1")
set_source_files_properties("../3rd_party/kiero2/kiero_d3d11.cpp" PROPERTIES COMPILE_DEFINITIONS "KIERO_IMPL_FIRST_SLOT=2")
set_source_files_properties("../3rd_party/kiero2/kiero_d3d12.cpp" PROPERTIES COMPILE_DEFINITIONS "KIERO_IMPL_FIRST_SLOT=3")
set_source_files_properties("../3rd_party/kiero2/kiero_opengl.cpp" PROPERTIES COMPILE_DEFINITIONS "KIERO_IMPL_FIRST_SLOT=4")

set(OP_COM_GENERATED_DIR "${CMAKE_CURRENT_BINARY_DIR}/com_generated")
set(OP_COM_I_C "${OP_COM_GENERATED_DIR}/op_i.c")
set(OP_COM_I_H "${OP_COM_GENERATED_DIR}/op_i.h")
set(OP_COM_TLB "${OP_COM_GENERATED_DIR}/op.tlb")
set(OP_COM_PROXY_C "${OP_COM_GENERATED_DIR}/op_p.c")
set(OP_COM_DLLDATA_C "${OP_COM_GENERATED_DIR}/dlldata.c")
set(OP_COM_GENERATED_FILES
  "${OP_COM_I_C}"
  "${OP_COM_I_H}"
  "${OP_COM_TLB}"
  "${OP_COM_PROXY_C}"
  "${OP_COM_DLLDATA_C}"
)
set_source_files_properties(${OP_COM_GENERATED_FILES} PROPERTIES GENERATED TRUE)
set(OP_COM_GENERATED_DIR "${OP_COM_GENERATED_DIR}" PARENT_SCOPE)
set(OP_COM_I_C "${OP_COM_I_C}" PARENT_SCOPE)
set(OP_COM_I_H "${OP_COM_I_H}" PARENT_SCOPE)
set(OP_COM_TLB "${OP_COM_TLB}" PARENT_SCOPE)
set(OP_COM_GENERATED_FILES "${OP_COM_GENERATED_FILES}" PARENT_SCOPE)

set(OP_COM_SOURCES
  "com/op.cpp"
  "com/OpAutomation.cpp"
  "com/stdafx.cpp"
  "com/compreg.cpp"
  "com/dllmain.cpp"
  "${OP_COM_I_C}"
  "${CMAKE_CURRENT_SOURCE_DIR}/com/op.def"
  "${CMAKE_CURRENT_SOURCE_DIR}/com/op.rc"
)

set(OP_C_API_SOURCES
  "c_api/op_c_api.cpp"
)

set(OP_COMMON_DEFINITIONS
  WIN32
  _WINDOWS
  _CRT_NON_CONFORMING_SWPRINTFS
  _CRT_SECURE_NO_WARNINGS
  _SILENCE_CXX17_ADAPTOR_TYPEDEFS_DEPRECATION_WARNING
  _UNICODE
  UNICODE
  OP_VERSION="0.4.7.0"
)
if(DEFINED OP_ARCH_DEFINITION AND NOT OP_ARCH_DEFINITION STREQUAL "")
  list(APPEND OP_COMMON_DEFINITIONS "${OP_ARCH_DEFINITION}")
endif()

set(OP_PRIVATE_INCLUDE_DIRS
  "${PROJECT_BINARY_DIR}"
  "${PROJECT_SOURCE_DIR}/include"
  "${CMAKE_CURRENT_SOURCE_DIR}"
  "${CMAKE_CURRENT_SOURCE_DIR}/runtime"
  "${CMAKE_CURRENT_SOURCE_DIR}/window"
  "${CMAKE_CURRENT_SOURCE_DIR}/image"
  "${CMAKE_CURRENT_SOURCE_DIR}/network"
  "${CMAKE_CURRENT_SOURCE_DIR}/ocr"
  "${CMAKE_CURRENT_SOURCE_DIR}/opencv"
  "${CMAKE_CURRENT_SOURCE_DIR}/yolo"
  "${CMAKE_CURRENT_SOURCE_DIR}/memory"
  "${CMAKE_CURRENT_SOURCE_DIR}/ipc"
  "${CMAKE_CURRENT_SOURCE_DIR}/hook"
  "${CMAKE_CURRENT_SOURCE_DIR}/hook/vendor"
  "${PROJECT_SOURCE_DIR}/3rd_party/kiero2"
  "${BLACKBONE_INCLUDE_DIR}"
  "${BLACKBONE_INCLUDE_DIR}/3rd_party"
)
if(OP_HAS_OPENCV)
  list(APPEND OP_PRIVATE_INCLUDE_DIRS "${OP_OPENCV_INCLUDE_DIR}")
endif()
list(APPEND OP_PRIVATE_INCLUDE_DIRS "${OP_COM_GENERATED_DIR}")

add_library(libop)
target_sources(libop PRIVATE ${OP_LIBOP_SOURCES})
target_compile_definitions(libop PRIVATE ${OP_COMMON_DEFINITIONS} U_STATIC_IMPLEMENTATION)
target_include_directories(libop
  PUBLIC "${PROJECT_SOURCE_DIR}/include"
  PRIVATE ${OP_PRIVATE_INCLUDE_DIRS}
)

add_custom_command(
  OUTPUT ${OP_COM_GENERATED_FILES}
  COMMAND ${CMAKE_COMMAND} -E make_directory "${OP_COM_GENERATED_DIR}"
  COMMAND midl "${CMAKE_CURRENT_SOURCE_DIR}/com/op.idl"
    /iid "${OP_COM_I_C}" /env win32 /h "${OP_COM_I_H}" /W1 /char signed
    /tlb "${OP_COM_TLB}" /Oicf /target "NT60" /D "NDEBUG" /robust /nologo
    /proxy "${OP_COM_PROXY_C}" /dlldata "${OP_COM_DLLDATA_C}"
  DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/com/op.idl
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)
set_source_files_properties("${CMAKE_CURRENT_SOURCE_DIR}/com/op.rc" PROPERTIES OBJECT_DEPENDS "${OP_COM_TLB}")

add_library(${op_com} SHARED)
target_sources(${op_com} PRIVATE ${OP_LIBOP_SOURCES} ${OP_COM_SOURCES})
target_compile_definitions(${op_com} PRIVATE
  ${OP_COMMON_DEFINITIONS}
  _USRDLL
  _WINDLL
  OP_EXPORTS
)
target_include_directories(${op_com}
  PRIVATE ${OP_PRIVATE_INCLUDE_DIRS}
)
set_target_properties(${op_com} PROPERTIES LINK_FLAGS "/DEF:\"${CMAKE_CURRENT_SOURCE_DIR}/com/op.def\" /NODEFAULTLIB:\"mfc110d\"")

add_library(${op_c_api} SHARED)
target_sources(${op_c_api} PRIVATE ${OP_C_API_SOURCES})
target_compile_definitions(${op_c_api} PRIVATE
  ${OP_COMMON_DEFINITIONS}
  U_STATIC_IMPLEMENTATION
  OP_C_API_EXPORTS
)
target_include_directories(${op_c_api}
  PUBLIC "${PROJECT_SOURCE_DIR}/include"
  PRIVATE ${OP_PRIVATE_INCLUDE_DIRS}
)

if(enable_wgc)
  target_compile_definitions(libop PRIVATE OP_ENABLE_WGC)
  target_compile_definitions(${op_com} PRIVATE OP_ENABLE_WGC)
  target_compile_definitions(${op_c_api} PRIVATE OP_ENABLE_WGC)
endif()

if(enable_wgc AND MSVC)
  set_source_files_properties(
    "binding/BindingSession.cpp"
    "capture/backends/WgcCapture.cpp"
    PROPERTIES
      COMPILE_OPTIONS "/std:c++20;/D_SILENCE_EXPERIMENTAL_COROUTINE_DEPRECATION_WARNINGS"
  )
endif()

target_link_libraries(${op_com} PRIVATE "${BLACKBONE_LIBRARY}")
target_link_libraries(${op_c_api} PRIVATE libop "${BLACKBONE_LIBRARY}")

if(enable_wgc)
  target_link_libraries(libop PRIVATE windowsapp runtimeobject.lib)
  target_link_libraries(${op_com} PRIVATE windowsapp runtimeobject.lib)
  target_link_libraries(${op_c_api} PRIVATE windowsapp runtimeobject.lib)
endif()

# MinHook 通过 vcpkg 提供的 imported target 链接,头文件与库均由 minhook::minhook
# 的 INTERFACE_INCLUDE_DIRECTORIES / IMPORTED_LOCATION 提供。链到所有编译
# MinHook 源码(kiero.cpp / InputHook.cpp)的 target,确保 include 目录可达。
find_package(minhook CONFIG REQUIRED)
target_link_libraries(libop       PRIVATE minhook::minhook)
target_link_libraries(${op_com}   PRIVATE minhook::minhook)
target_link_libraries(${op_c_api} PRIVATE minhook::minhook)

target_link_libraries(libop PRIVATE dwmapi.lib)
target_link_libraries(${op_com} PRIVATE dwmapi.lib)
target_link_libraries(${op_c_api} PRIVATE dwmapi.lib)

if(OP_HAS_OPENCV)
  target_link_libraries(libop PRIVATE ${OP_OPENCV_LINK_LIBS})
  target_link_libraries(${op_com} PRIVATE ${OP_OPENCV_LINK_LIBS})
  target_link_libraries(${op_c_api} PRIVATE ${OP_OPENCV_LINK_LIBS})
endif()

set(OP_HAS_OPENCV "${OP_HAS_OPENCV}" PARENT_SCOPE)

install(FILES "${PROJECT_SOURCE_DIR}/include/libop.h" DESTINATION "${PROJECT_SOURCE_DIR}/include")
install(FILES "${PROJECT_SOURCE_DIR}/include/op_c_api.h" DESTINATION "${PROJECT_SOURCE_DIR}/include")
if(CMAKE_CL_64)
  install(TARGETS ${op_com} RUNTIME DESTINATION "${PROJECT_SOURCE_DIR}/bin/x64")
  install(TARGETS ${op_com} ARCHIVE DESTINATION "${PROJECT_SOURCE_DIR}/lib/x64")
  install(TARGETS ${op_c_api} RUNTIME DESTINATION "${PROJECT_SOURCE_DIR}/bin/x64")
  install(TARGETS ${op_c_api} ARCHIVE DESTINATION "${PROJECT_SOURCE_DIR}/lib/x64")
else()
  install(TARGETS ${op_com} RUNTIME DESTINATION "${PROJECT_SOURCE_DIR}/bin/x86")
  install(TARGETS ${op_com} ARCHIVE DESTINATION "${PROJECT_SOURCE_DIR}/lib/x86")
  install(TARGETS ${op_c_api} RUNTIME DESTINATION "${PROJECT_SOURCE_DIR}/bin/x86")
  install(TARGETS ${op_c_api} ARCHIVE DESTINATION "${PROJECT_SOURCE_DIR}/lib/x86")
endif()
