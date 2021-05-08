# 1.cmake version
cmake_minimum_required(VERSION 3.0.0)



# 3.include path
#INCLUDE_DIRECTORIES(
#    include
#)
<<<<<<< HEAD
#set(CMAKE_CXX_STANDARD 17)
#set(CMAKE_CXX_STANDARD_REQUIRED True)
if (MSVC_VERSION GREATER_EQUAL "1900")
    include(CheckCXXCompilerFlag)
    CHECK_CXX_COMPILER_FLAG("/std:c++17" _cpp_latest_flag_supported)
    if (_cpp_latest_flag_supported)
        add_compile_options("/std:c++17")
    endif()
endif()



=======
>>>>>>> 058014ddff8f53317da31bf98806f3a189196721

set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /DEF:op.def /NODEFAULTLIB")

 

# set_target_properties(target PROPERTIES VcpkgEnabled "true")

# 4.source directory
AUX_SOURCE_DIRECTORY(src DIR_SRCS)

#LINK_DIRECTORIES("/3rd_party/lib/x86")
# 5.set environment variable
# find_package(OpenCV REQUIRED)
# include_directories(libop ${OpenCV_INCLUDE_DIRS})


#find_package(minhook CONFIG REQUIRED)
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

"./winapi/injecter.cpp"
"./winapi/memoryEx.cpp"
"./winapi/query_api.cpp"
"./winapi/winapi.cpp"
"../3rd_party/src/kiero.cpp"
"libop.cpp"
)



ADD_DEFINITIONS(-D WIN32)
ADD_DEFINITIONS(-D _WINDOWS)
ADD_DEFINITIONS(-D _USRDLL)
ADD_DEFINITIONS(-D _CRT_NON_CONFORMING_SWPRINTFS)
ADD_DEFINITIONS(-D _CRT_SECURE_NO_WARNINGS)
ADD_DEFINITIONS(-D _SILENCE_CXX17_ADAPTOR_TYPEDEFS_DEPRECATION_WARNING)
ADD_DEFINITIONS(-D _WINDLL)
ADD_DEFINITIONS(-D _UNICODE)
ADD_DEFINITIONS(-D UNICODE)
 

# 6.add lib file
add_library(libop SHARED ${SRC_FILES})

target_include_directories(libop PUBLIC
"${PROJECT_BINARY_DIR}"
"${CMAKE_CURRENT_SOURCE_DIR}"
)

include_directories(
  ./core
)

# 7.add link library
TARGET_LINK_LIBRARIES(libop BlackBone.lib)
TARGET_LINK_LIBRARIES(libop minhook.lib)


# target_link_libraries(libop  ${OpenCV_LIBS})
# target_link_libraries(libop msvcrt.lib)


 
install(FILES libop.h DESTINATION "${PROJECT_SOURCE_DIR}/include")
IF(CMAKE_CL_64)
  install(TARGETS libop RUNTIME DESTINATION "${PROJECT_SOURCE_DIR}/bin/x64")
  install(TARGETS libop ARCHIVE DESTINATION "${PROJECT_SOURCE_DIR}/lib/x64")
ELSE(CMAKE_CL_64)
  install(TARGETS libop RUNTIME DESTINATION "${PROJECT_SOURCE_DIR}/bin/x86")
  install(TARGETS libop ARCHIVE DESTINATION "${PROJECT_SOURCE_DIR}/lib/x86")
ENDIF(CMAKE_CL_64)




