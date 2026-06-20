cmake_minimum_required(VERSION 3.10)

if(CMAKE_SIZEOF_VOID_P EQUAL 8)
  set(OP_GTEST_STATIC_TRIPLET "x64-windows-static")
else()
  set(OP_GTEST_STATIC_TRIPLET "x86-windows-static")
endif()

set(OP_GTEST_STATIC_OK FALSE)
set(OP_GTEST_INCLUDE_DIR "")
set(OP_GTEST_LINK_LIBS "")
set(OP_VCPKG_INSTALLED_DIR "")

if(DEFINED VCPKG_INSTALLED_DIR)
  set(OP_VCPKG_INSTALLED_DIR "${VCPKG_INSTALLED_DIR}")
elseif(DEFINED _VCPKG_INSTALLED_DIR)
  set(OP_VCPKG_INSTALLED_DIR "${_VCPKG_INSTALLED_DIR}")
elseif(DEFINED ENV{VCPKG_ROOT})
  set(OP_VCPKG_INSTALLED_DIR "$ENV{VCPKG_ROOT}/installed")
elseif(DEFINED CMAKE_TOOLCHAIN_FILE)
  get_filename_component(_op_toolchain_dir "${CMAKE_TOOLCHAIN_FILE}" DIRECTORY)
  get_filename_component(_op_vcpkg_scripts_dir "${_op_toolchain_dir}" DIRECTORY)
  get_filename_component(_op_vcpkg_root "${_op_vcpkg_scripts_dir}" DIRECTORY)
  set(OP_VCPKG_INSTALLED_DIR "${_op_vcpkg_root}/installed")
endif()

if(OP_VCPKG_INSTALLED_DIR)
  file(TO_CMAKE_PATH "${OP_VCPKG_INSTALLED_DIR}" OP_VCPKG_INSTALLED_DIR)
  set(OP_GTEST_STATIC_ROOT "${OP_VCPKG_INSTALLED_DIR}/${OP_GTEST_STATIC_TRIPLET}")
  set(OP_GTEST_STATIC_INCLUDE "${OP_GTEST_STATIC_ROOT}/include")
  set(OP_GTEST_STATIC_LIB_RELEASE "${OP_GTEST_STATIC_ROOT}/lib/gtest.lib")

  if(EXISTS "${OP_GTEST_STATIC_INCLUDE}/gtest/gtest.h"
     AND EXISTS "${OP_GTEST_STATIC_LIB_RELEASE}")
    set(OP_GTEST_INCLUDE_DIR "${OP_GTEST_STATIC_INCLUDE}")
    set(OP_GTEST_LINK_LIBS "${OP_GTEST_STATIC_LIB_RELEASE}")
    set(OP_GTEST_STATIC_OK TRUE)
    message(STATUS "Using static GoogleTest: ${OP_GTEST_STATIC_LIB_RELEASE}")
  endif()
endif()

if(NOT OP_GTEST_STATIC_OK)
  find_package(GTest CONFIG REQUIRED)
  set(OP_GTEST_LINK_LIBS GTest::gtest)
endif()

set(OP_HAS_OPENCV_SOURCES FALSE)
set(OP_OPENCV_INCLUDE_DIR "")
set(OP_OPENCV_LIBS "")
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

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../libop/opencv/TemplateMatcher.h"
   AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../libop/opencv/TemplateMatcher.cpp")
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
      set(_op_opencv_lib_dir "${_op_opencv_root}/x64/vc18/staticlib")
    else()
      set(_op_opencv_lib_dir "${_op_opencv_root}/x86/vc18/staticlib")
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
      _op_find_static_lib(_op_lib_path "${_op_opencv_lib_dir}" "${_op_lib_name}")
      if(_op_lib_path STREQUAL "")
        set(_op_all_libs_exist FALSE)
      else()
        list(APPEND _op_expected_libs "${_op_lib_path}")
      endif()
    endforeach()

    if(EXISTS "${OP_OPENCV_INCLUDE_DIR}/opencv2/core.hpp" AND _op_all_libs_exist)
      set(OP_HAS_OPENCV_SOURCES TRUE)
      set(OP_OPENCV_LIBS ${_op_expected_libs})
      message(STATUS "OpenCV tests enabled via manual static libraries: ${_op_opencv_lib_dir}")
    else()
      message(WARNING "OpenCV source adapter exists but required OpenCV headers or libraries were not found. Skipping OpenCV tests.")
    endif()
  else()
    message(WARNING "OpenCV source adapter exists but OPENCV_ROOT is not set. Skipping OpenCV tests.")
  endif()
endif()

set(OP_TEST_SOURCES
  "main.cpp"
  "test_support.cpp"
  "handle_compat_test.cpp"
  "core_algorithm_windows_test.cpp"
  "mouse_keyboard_test.cpp"
  "image_color_test.cpp"
  "wgc_test.cpp"
  "ocr_test.cpp"
  "yolo_test.cpp"
  "utils_test.cpp"
  "../libop/window/WindowLayout.cpp"
  "../libop/input/mouse/CursorShape.cpp"
  "${OP_COM_I_C}"
)
set_source_files_properties("${OP_COM_I_C}" PROPERTIES GENERATED TRUE)

if(OP_HAS_OPENCV_SOURCES AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/opencv_test.cpp")
  list(APPEND OP_TEST_SOURCES
    "opencv_test.cpp"
    "../libop/opencv/OpenCvHelpers.cpp"
    "../libop/opencv/TemplateMatcher.cpp"
  )
endif()

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/keypad/edit_control_keypad_test.cpp")
  list(APPEND OP_TEST_SOURCES "keypad/edit_control_keypad_test.cpp")
endif()

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/keypad/keypad_message_test.cpp")
  list(APPEND OP_TEST_SOURCES "keypad/keypad_message_test.cpp")
endif()

add_executable(op_test)
target_sources(op_test PRIVATE ${OP_TEST_SOURCES})
target_compile_definitions(op_test PRIVATE _UNICODE UNICODE _CMAKE_BUILD)
if(DEFINED OP_ARCH_DEFINITION AND NOT OP_ARCH_DEFINITION STREQUAL "")
  target_compile_definitions(op_test PRIVATE "${OP_ARCH_DEFINITION}")
endif()
if(enable_wgc)
  target_compile_definitions(op_test PRIVATE OP_ENABLE_WGC)
endif()

target_include_directories(op_test PRIVATE
  "${CMAKE_CURRENT_SOURCE_DIR}"
  "${PROJECT_SOURCE_DIR}/include"
  "${PROJECT_SOURCE_DIR}/libop"
  "${PROJECT_SOURCE_DIR}/libop/runtime"
  "${PROJECT_SOURCE_DIR}/libop/window"
  "${PROJECT_SOURCE_DIR}/libop/image"
  "${PROJECT_SOURCE_DIR}/libop/ocr"
  "${PROJECT_SOURCE_DIR}/libop/opencv"
  "${OP_COM_GENERATED_DIR}"
  "${PROJECT_SOURCE_DIR}/3rd_party/include"
  "${PROJECT_SOURCE_DIR}/3rd_party/include/3rd_party"
  "${BLACKBONE_INCLUDE_DIR}"
  "${BLACKBONE_INCLUDE_DIR}/3rd_party"
)
if(OP_GTEST_INCLUDE_DIR)
  target_include_directories(op_test PRIVATE "${OP_GTEST_INCLUDE_DIR}")
endif()

target_link_libraries(op_test PRIVATE ${op_com} ${OP_GTEST_LINK_LIBS} dinput8.lib dxguid.lib dwmapi.lib)

if(OP_HAS_OPENCV_SOURCES)
  target_include_directories(op_test PRIVATE ${OP_OPENCV_INCLUDE_DIR})
  target_link_libraries(op_test PRIVATE ${OP_OPENCV_LIBS})
endif()

add_test(NAME op_test COMMAND op_test)
set_tests_properties(op_test
  PROPERTIES ENVIRONMENT "PATH=$<TARGET_FILE_DIR:${op_com}>;$ENV{PATH}")

if(CMAKE_CL_64)
  install(TARGETS op_test DESTINATION "${PROJECT_SOURCE_DIR}/bin/x64")
else()
  install(TARGETS op_test DESTINATION "${PROJECT_SOURCE_DIR}/bin/x86")
endif()

set(OP_DEBUG_SOURCES
  "debug_runner/main_test.cpp"
)
if(OP_HAS_OPENCV_SOURCES AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/debug_runner/main_test.cpp")
  add_executable(op_debug)
  target_sources(op_debug PRIVATE ${OP_DEBUG_SOURCES})
  target_compile_definitions(op_debug PRIVATE _UNICODE UNICODE _CMAKE_BUILD)
  target_include_directories(op_debug PRIVATE
    "${PROJECT_SOURCE_DIR}/include"
    "${PROJECT_SOURCE_DIR}/libop"
    "${PROJECT_SOURCE_DIR}/libop/runtime"
    "${PROJECT_SOURCE_DIR}/libop/window"
    "${PROJECT_SOURCE_DIR}/libop/image"
    "${PROJECT_SOURCE_DIR}/libop/ocr"
    "${PROJECT_SOURCE_DIR}/libop/opencv"
  )
  target_link_libraries(op_debug PRIVATE ${op_com})

  set_target_properties(op_debug PROPERTIES
    VS_DEBUGGER_WORKING_DIRECTORY "$<TARGET_FILE_DIR:op_debug>"
    VS_DEBUGGER_COMMAND_ARGUMENTS "smoke"
  )

  if(CMAKE_CL_64)
    install(TARGETS op_debug DESTINATION "${PROJECT_SOURCE_DIR}/bin/x64")
  else()
    install(TARGETS op_debug DESTINATION "${PROJECT_SOURCE_DIR}/bin/x86")
  endif()
endif()
