cmake_minimum_required(VERSION 3.0)

if (MSVC_VERSION GREATER_EQUAL "1900")
    include(CheckCXXCompilerFlag)
    CHECK_CXX_COMPILER_FLAG("/std:c++latest" _cpp_latest_flag_supported)
    if (_cpp_latest_flag_supported)
        add_compile_options("/std:c++latest")
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

add_library(op SHARED ${COM_SRC_FILES} ${CMAKE_CURRENT_SOURCE_DIR}/op.rc)

target_link_libraries(op libop)

SET_TARGET_PROPERTIES(op PROPERTIES LINK_FLAGS "/DEF:\"op.def\" /NODEFAULTLIB:\"mfc110d\"")

add_custom_command(TARGET op PRE_BUILD  COMMAND midl op.idl /iid "op_i.c" /env win32 /h "op_i.h" /W1 /char signed /tlb "op.tlb" /Oicf /target "NT60" /D "NDEBUG" /robust /nologo /proxy "op_p.c"  WORKING_DIRECTORY  ${CMAKE_CURRENT_SOURCE_DIR})


install(TARGETS op RUNTIME DESTINATION "${PROJECT_SOURCE_DIR}/bin")
install(TARGETS op ARCHIVE DESTINATION "${PROJECT_SOURCE_DIR}/lib")
