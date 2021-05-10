cmake_minimum_required(VERSION 3.0)
if (MSVC_VERSION GREATER_EQUAL "1900")
    include(CheckCXXCompilerFlag)
    CHECK_CXX_COMPILER_FLAG("/std:c++17" _cpp_latest_flag_supported)
    if (_cpp_latest_flag_supported)
        add_compile_options("/std:c++17")
    endif()
endif()

 


ADD_DEFINITIONS(-D WIN32)
ADD_DEFINITIONS(-D _WINDOWS)
ADD_DEFINITIONS(-D _USRDLL)
ADD_DEFINITIONS(-D _CRT_NON_CONFORMING_SWPRINTFS)
ADD_DEFINITIONS(-D _CRT_SECURE_NO_WARNINGS)
ADD_DEFINITIONS(-D _SILENCE_CXX17_ADAPTOR_TYPEDEFS_DEPRECATION_WARNING)
ADD_DEFINITIONS(-D _WINDLL)
ADD_DEFINITIONS(-D _UNICODE)
ADD_DEFINITIONS(-D UNICODE)

SET(COM_SRC_FILES 
"../libop/core/globalVar.cpp"
"op.cpp"
"opinterface.cpp"
"stdafx.cpp"
"compreg.cpp"
"dllmain.cpp"
"op_i.c"
"op.def")

IF(CMAKE_CL_64)
    set(OUTPUT_NAME op_x64)
ELSE(CMAKE_CL_64)
    set(OUTPUT_NAME op_x86)
ENDIF(CMAKE_CL_64)

add_library(${OUTPUT_NAME} SHARED ${COM_SRC_FILES} ${CMAKE_CURRENT_SOURCE_DIR}/op.rc)

target_link_libraries(${OUTPUT_NAME} libop)

SET_TARGET_PROPERTIES(${OUTPUT_NAME} PROPERTIES LINK_FLAGS "/DEF:\"op.def\" /NODEFAULTLIB:\"mfc110d\"")

add_custom_command(TARGET ${OUTPUT_NAME} PRE_BUILD  COMMAND midl op.idl /iid "op_i.c" /env win32 /h "op_i.h" /W1 /char signed /tlb "op.tlb" /Oicf /target "NT60" /D "NDEBUG" /robust /nologo /proxy "op_p.c"  WORKING_DIRECTORY  ${CMAKE_CURRENT_SOURCE_DIR})


IF(CMAKE_CL_64)
    install(TARGETS ${OUTPUT_NAME} RUNTIME DESTINATION "${PROJECT_SOURCE_DIR}/bin/x64")
    install(TARGETS ${OUTPUT_NAME} ARCHIVE DESTINATION "${PROJECT_SOURCE_DIR}/lib/x64")
ELSE(CMAKE_CL_64)
    install(TARGETS ${OUTPUT_NAME} RUNTIME DESTINATION "${PROJECT_SOURCE_DIR}/bin/x86")
    install(TARGETS ${OUTPUT_NAME} ARCHIVE DESTINATION "${PROJECT_SOURCE_DIR}/lib/x86")
ENDIF(CMAKE_CL_64)