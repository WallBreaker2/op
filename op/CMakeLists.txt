# 1.cmake version
cmake_minimum_required(VERSION 3.0.0)



# 3.include path
#INCLUDE_DIRECTORIES(
#    include
#)

if (MSVC_VERSION GREATER_EQUAL "1900")
    include(CheckCXXCompilerFlag)
    CHECK_CXX_COMPILER_FLAG("/std:c++latest" _cpp_latest_flag_supported)
    if (_cpp_latest_flag_supported)
        add_compile_options("/std:c++latest")
    endif()
endif()




set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /DEF:op.def /NODEFAULTLIB")

 



# set_target_properties(target PROPERTIES VcpkgEnabled "true")

# 4.source directory
AUX_SOURCE_DIRECTORY(src DIR_SRCS)

#LINK_DIRECTORIES("/3rd_party/lib/x86")
# 5.set environment variable
# find_package(OpenCV REQUIRED)
# include_directories(${PROJECT_NAME} ${OpenCV_INCLUDE_DIRS})

MESSAGE(STATUS ${Opencv})


find_library(BLACKBONE_LIB blackbone ./3rd_party/lib/x86 NO_DEFAULT_PATH)

find_library(minhook_lib libminhook.x86 ./3rd_party/lib/x86 NO_DEFAULT_PATH)

#find_package(minhook CONFIG REQUIRED)
message(STATUS ${BLACKBONE_LIB})
#message(STATUS ${minhook_lib})




SET(SRC_FILES 
	"./background/bkbase.cpp" 
  "./background/bkdx_gl.cpp" 
  "./background/bkgdi.cpp" 
  "./background/bkkeypad.cpp" 
  "./background/bkmouse.cpp" 
  "./background/IDisplay.cpp" 
  "./background/winkeypad.cpp" 
  "./background/xhook.cpp"


 
"./core/globalVar.cpp"
"./core/helpfunc.cpp"
"./core/pipe.cpp"


"./imageProc/imageloc.cpp"
"./imageProc/imageproc.cpp"

"./interface/libop.cpp"

"./winapi/injecter.cpp"
"./winapi/memoryEx.cpp"
"./winapi/query_api.cpp"
"./winapi/winapi.cpp"
"./3rd_party/src/kiero.cpp"
"op.cpp"
"opinterface.cpp"
"stdafx.cpp"
"compreg.cpp"
"dllmain.cpp"
"op_i.c"
"op.def"
)


#SET(LIBS "blackbone.lib")

ADD_DEFINITIONS(-D WIN32)
ADD_DEFINITIONS(-D _WINDOWS)
ADD_DEFINITIONS(-D _USRDLL)
ADD_DEFINITIONS(-D _CRT_NON_CONFORMING_SWPRINTFS)
ADD_DEFINITIONS(-D _CRT_SECURE_NO_WARNINGS)
ADD_DEFINITIONS(-D _SILENCE_CXX17_ADAPTOR_TYPEDEFS_DEPRECATION_WARNING)
ADD_DEFINITIONS(-D _WINDLL)
ADD_DEFINITIONS(-D _UNICODE)
ADD_DEFINITIONS(-D UNICODE)

# ADD_DEFINITIONS(-D APSTUDIO_INVOKED)
# 6.add executable file
add_library(${PROJECT_NAME} SHARED ${SRC_FILES} ${CMAKE_CURRENT_SOURCE_DIR}/op.rc)

target_include_directories(${PROJECT_NAME} PUBLIC
"${PROJECT_BINARY_DIR}"
"${CMAKE_CURRENT_SOURCE_DIR}"
)

# 7.add link library
TARGET_LINK_LIBRARIES(${PROJECT_NAME} ${BLACKBONE_LIB})
TARGET_LINK_LIBRARIES(${PROJECT_NAME} ${minhook_lib})
#target_link_libraries(${PROJECT_NAME} ${OPENCV_LIBRARIES})

target_link_libraries(${PROJECT_NAME}  ${OpenCV_LIBS})
target_link_libraries(${PROJECT_NAME} msvcrt.lib)


#target_link_libraries(${PROJECT_NAME}   minhook::minhook)



SET_TARGET_PROPERTIES(${PROJECT_NAME} PROPERTIES LINK_FLAGS "/DEF:\"op.def\" /NODEFAULTLIB:\"mfc110d\"")

add_custom_command(TARGET ${PROJECT_NAME} PRE_BUILD  COMMAND midl op.idl /iid "op_i.c" /env win32 /h "op_i.h" /W1 /char signed /tlb "op.tlb" /Oicf /target "NT60" /D "NDEBUG" /robust /nologo /proxy "op_p.c"  WORKING_DIRECTORY  ${CMAKE_CURRENT_SOURCE_DIR})

install(TARGETS ${PROJECT_NAME} RUNTIME DESTINATION "${PROJECT_SOURCE_DIR}/bin")
install(TARGETS ${PROJECT_NAME} ARCHIVE DESTINATION "${PROJECT_SOURCE_DIR}/lib")





