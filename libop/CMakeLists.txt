# 1.cmake version
cmake_minimum_required(VERSION 3.10)



# 3.include path
#INCLUDE_DIRECTORIES(
#    include
#)




set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /DEF:op.def /NODEFAULTLIB")

 

# set_target_properties(target PROPERTIES VcpkgEnabled "true")

# 4.source directory
AUX_SOURCE_DIRECTORY(./ DIR_SRCS)

#LINK_DIRECTORIES("/3rd_party/lib/x86")
# 5.set environment variable
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


#find_package(minhook CONFIG REQUIRED)
 #message(STATUS ${minhook_lib})




SET(SRC_FILES 
	"./background/opBackground.cpp" 
	"./background/displayInputHelper.cpp"

  "./background/display/opGDI.cpp" 
  "./background/display/opDXGI.cpp" 
  "./background/display/opDxGL.cpp" 
  "./background/display/IDisplay.cpp" 

  "./background/mouse/opMouseWin.cpp" 
  "./background/mouse/opMouseDx.cpp" 
  "./background/mouse/CursorShape.cpp" 

  "./background/keypad/bkkeypad.cpp" 
  "./background/keypad/winkeypad.cpp" 
  "./background/keypad/opKeypadDx.cpp" 
  
  "./background/Hook/DisplayHook.cpp"
  "./background/Hook/InputHook.cpp"
  "./background/Hook/InputHookClient.cpp"
  "./background/Hook/HookExport.cpp"
  "./background/Hook/opDx12Hook.cpp"

 
"./core/globalVar.cpp"
"./core/diaGuids.cpp"
"./core/helpfunc.cpp"
"./core/pipe.cpp"
"./core/opEnv.cpp"
"./core/win_version.cpp"
"./core/window_layout.cpp"


	"./imageProc/imageloc.cpp"
	"./imageProc/imageproc.cpp"
#	 "imageProc/tess_ocr.cpp"
	 "imageProc/OcrWrapper.cpp"
"./winapi/injecter.cpp"
"./winapi/memoryEx.cpp"
 "./winapi/query_api.cpp"
 "./winapi/winapi.cpp"
"../3rd_party/src/kiero.cpp"
"libop.cpp"
)

if(enable_wgc)
  list(APPEND SRC_FILES "./background/display/opWGC.cpp")
endif()

if(OP_HAS_OPENCV AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/opencv/OpenCvModule.cpp")
  list(APPEND SRC_FILES
    "./opencv/OpenCvBridge.cpp"
    "./opencv/OpenCvHelpers.cpp"
    "./opencv/OpenCvModule.cpp"
  )
endif()

SET(COM_SRC_FILES 
"./core/globalVar.cpp"
"./com/op.cpp"
"./com/opinterface.cpp"
"./com/stdafx.cpp"
"./com/compreg.cpp"
"./com/dllmain.cpp"
"./com/op_i.c"
"./com/op.def")

SET(C_API_SRC_FILES
"./c_api/op_c_api.cpp")






ADD_DEFINITIONS(-DUNICODE -D_UNICODE)
ADD_DEFINITIONS(-D WIN32)
ADD_DEFINITIONS(-D _WINDOWS)
ADD_DEFINITIONS(-D _USRDLL)
ADD_DEFINITIONS(-D _CRT_NON_CONFORMING_SWPRINTFS)
ADD_DEFINITIONS(-D _CRT_SECURE_NO_WARNINGS)
ADD_DEFINITIONS(-D _SILENCE_CXX17_ADAPTOR_TYPEDEFS_DEPRECATION_WARNING)
ADD_DEFINITIONS(-D _WINDLL)
ADD_DEFINITIONS(-D _UNICODE)
ADD_DEFINITIONS(-D UNICODE)
ADD_DEFINITIONS(-DOP_EXPORTS)
ADD_DEFINITIONS(-DOP_VERSION="0.4.7.0")
 

# 6.add lib file
#add_library(libop SHARED  ${SRC_FILES})

add_library(libop ${SRC_FILES})
target_compile_definitions(libop PRIVATE U_STATIC_IMPLEMENTATION)

if(enable_wgc AND MSVC)
  set_source_files_properties(
    "./background/opBackground.cpp"
    "./background/display/opWGC.cpp"
    PROPERTIES
      COMPILE_OPTIONS "/std:c++20;/D_SILENCE_EXPERIMENTAL_COROUTINE_DEPRECATION_WARNINGS"
  )
endif()

# ----------op_x86/x64

add_custom_command(
  OUTPUT
    ${CMAKE_CURRENT_SOURCE_DIR}/com/op_i.c
    ${CMAKE_CURRENT_SOURCE_DIR}/com/op_i.h
    ${CMAKE_CURRENT_SOURCE_DIR}/com/op.tlb
    ${CMAKE_CURRENT_SOURCE_DIR}/op_p.c
  COMMAND midl ./com/op.idl /iid
  "./com/op_i.c" /env win32 /h "./com/op_i.h" /W1 /char signed /tlb 
  "./com/op.tlb" /Oicf /target "NT60" /D "NDEBUG" /robust /nologo /proxy "op_p.c"
  DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/com/op.idl
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
  )

add_library(${op_com} SHARED ${SRC_FILES} ${COM_SRC_FILES} ${CMAKE_CURRENT_SOURCE_DIR}/./com/op.rc)
SET_TARGET_PROPERTIES(${op_com} PROPERTIES LINK_FLAGS "/DEF:\"op.def\" /NODEFAULTLIB:\"mfc110d\"")

add_library(${op_c_api} SHARED ${C_API_SRC_FILES})
target_compile_definitions(${op_c_api} PRIVATE OP_C_API_EXPORTS)

if(enable_wgc AND MSVC)
  set_source_files_properties(
    "./background/opBackground.cpp"
    "./background/display/opWGC.cpp"
    PROPERTIES
      COMPILE_OPTIONS "/std:c++20;/D_SILENCE_EXPERIMENTAL_COROUTINE_DEPRECATION_WARNINGS"
  )
endif()
# add_custom_command(TARGET ${op_com} PRE_BUILD  COMMAND midl ./com/op.idl /iid "./com/op_i.c" /env win32 /h "./com/op_i.h" /W1 /char signed /tlb "./com/op.tlb" /Oicf /target "NT60" /D "NDEBUG" /robust /nologo /proxy "op_p.c"  WORKING_DIRECTORY  ${CMAKE_CURRENT_SOURCE_DIR})




target_include_directories(${op_com} PUBLIC
"${PROJECT_BINARY_DIR}"
"${CMAKE_CURRENT_SOURCE_DIR}"
)

target_include_directories(${op_c_api} PUBLIC
"${PROJECT_SOURCE_DIR}/include"
"${PROJECT_BINARY_DIR}"
"${CMAKE_CURRENT_SOURCE_DIR}"
)

target_include_directories(libop PUBLIC
"${PROJECT_BINARY_DIR}"
"${CMAKE_CURRENT_SOURCE_DIR}"
)

include_directories(
  ./
  ./core
)

if(OP_HAS_OPENCV)
  include_directories(${OP_OPENCV_INCLUDE_DIR})
endif()

## for tesseract lib
# find_package(Tesseract)
# include_directories(${Tesseract_INCLUDE_DIRS})
# target_link_libraries(${op_com} Tesseract::libtesseract)


# 7.add link library
# BlackBone 仍通过外部提供的 BlackBone.lib 链接
TARGET_LINK_LIBRARIES(${op_com} "${BLACKBONE_LIBRARY}")
TARGET_LINK_LIBRARIES(${op_c_api} libop "${BLACKBONE_LIBRARY}")
if(enable_wgc)
  TARGET_LINK_LIBRARIES(${op_com} windowsapp runtimeobject.lib)
  TARGET_LINK_LIBRARIES(${op_c_api} windowsapp runtimeobject.lib)
endif()

# MinHook 使用 vcpkg static triplet 的静态库，避免运行时依赖 minhook.x*.dll。
set(OP_MINHOOK_LINKED FALSE)
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
    set(OP_MINHOOK_STATIC_TRIPLET "x64-windows-static")
    set(OP_MINHOOK_LIB_RELEASE_NAME "minhook.x64.lib")
    set(OP_MINHOOK_LIB_DEBUG_NAME "minhook.x64d.lib")
  else()
    set(OP_MINHOOK_STATIC_TRIPLET "x86-windows-static")
    set(OP_MINHOOK_LIB_RELEASE_NAME "minhook.x32.lib")
    set(OP_MINHOOK_LIB_DEBUG_NAME "minhook.x32d.lib")
  endif()

  set(OP_MINHOOK_LIB_RELEASE "${OP_VCPKG_INSTALLED_DIR}/${OP_MINHOOK_STATIC_TRIPLET}/lib/${OP_MINHOOK_LIB_RELEASE_NAME}")
  set(OP_MINHOOK_LIB_DEBUG "${OP_VCPKG_INSTALLED_DIR}/${OP_MINHOOK_STATIC_TRIPLET}/debug/lib/${OP_MINHOOK_LIB_DEBUG_NAME}")

  if (EXISTS "${OP_MINHOOK_LIB_RELEASE}")
    TARGET_LINK_LIBRARIES(${op_com} "${OP_MINHOOK_LIB_RELEASE}")
    TARGET_LINK_LIBRARIES(${op_c_api} "${OP_MINHOOK_LIB_RELEASE}")
    set(OP_MINHOOK_LINKED TRUE)
    message(STATUS "Using static MinHook library: ${OP_MINHOOK_LIB_RELEASE}")
  endif()
endif()

if (NOT OP_MINHOOK_LINKED)
  # 兼容历史环境：优先 package target，最终退回库名
  find_package(minhook CONFIG QUIET)
  if (minhook_FOUND)
    TARGET_LINK_LIBRARIES(${op_com} minhook::minhook)
    TARGET_LINK_LIBRARIES(${op_c_api} minhook::minhook)
  else()
    TARGET_LINK_LIBRARIES(${op_com} minhook.lib)
    TARGET_LINK_LIBRARIES(${op_c_api} minhook.lib)
  endif()
endif()

if(enable_wgc)
  target_link_libraries(libop windowsapp runtimeobject.lib)
endif()
target_link_libraries(libop dwmapi.lib)
target_link_libraries(${op_com} dwmapi.lib)
target_link_libraries(${op_c_api} dwmapi.lib)

if(OP_HAS_OPENCV)
  target_link_libraries(libop ${OP_OPENCV_LINK_LIBS})
  target_link_libraries(${op_com} ${OP_OPENCV_LINK_LIBS})
  target_link_libraries(${op_c_api} ${OP_OPENCV_LINK_LIBS})
endif()

set(OP_HAS_OPENCV "${OP_HAS_OPENCV}" PARENT_SCOPE)

# target_link_libraries(libop  ${OpenCV_LIBS})
# target_link_libraries(libop msvcrt.lib)


 
install(FILES libop.h DESTINATION "${PROJECT_SOURCE_DIR}/include")
install(FILES "${PROJECT_SOURCE_DIR}/include/op_c_api.h" DESTINATION "${PROJECT_SOURCE_DIR}/include")
IF(CMAKE_CL_64)
  #install(TARGETS libop RUNTIME DESTINATION "${PROJECT_SOURCE_DIR}/bin/x64")
  #install(TARGETS libop ARCHIVE DESTINATION "${PROJECT_SOURCE_DIR}/lib/x64")
  install(TARGETS ${op_com} RUNTIME DESTINATION "${PROJECT_SOURCE_DIR}/bin/x64")
  install(TARGETS ${op_com} ARCHIVE DESTINATION "${PROJECT_SOURCE_DIR}/lib/x64")
  install(TARGETS ${op_c_api} RUNTIME DESTINATION "${PROJECT_SOURCE_DIR}/bin/x64")
  install(TARGETS ${op_c_api} ARCHIVE DESTINATION "${PROJECT_SOURCE_DIR}/lib/x64")
ELSE(CMAKE_CL_64)
  #install(TARGETS libop RUNTIME DESTINATION "${PROJECT_SOURCE_DIR}/bin/x86")
  #install(TARGETS libop ARCHIVE DESTINATION "${PROJECT_SOURCE_DIR}/lib/x86")
  install(TARGETS ${op_com} RUNTIME DESTINATION "${PROJECT_SOURCE_DIR}/bin/x86")
  install(TARGETS ${op_com} ARCHIVE DESTINATION "${PROJECT_SOURCE_DIR}/lib/x86")
  install(TARGETS ${op_c_api} RUNTIME DESTINATION "${PROJECT_SOURCE_DIR}/bin/x86")
  install(TARGETS ${op_c_api} ARCHIVE DESTINATION "${PROJECT_SOURCE_DIR}/lib/x86")
ENDIF(CMAKE_CL_64)
