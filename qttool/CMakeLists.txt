cmake_minimum_required(VERSION 3.0)

ADD_DEFINITIONS(-D _UNICODE)
ADD_DEFINITIONS(-D UNICODE)

ADD_DEFINITIONS(-D _CMAKE_BUILD)
ADD_DEFINITIONS(-D WIN32)
ADD_DEFINITIONS(-D _WINDOWS)

set(CMAKE_CXX_STANDARD 17)
#---------------add qt--------------
 set(CMAKE_PREFIX_PATH "G:\\Qt\\5.13.2\\msvc2017")
 include_directories("G:\\Qt\\5.13.2\\msvc2017include")
 find_package(Qt5 COMPONENTS Widgets REQUIRED)
 set(CMAKE_INCLUDE_CURRENT_DIR ON)
 set(CMAKE_AUTOMOC ON)
 set(CMAKE_AUTORCC ON)
 set(CMAKE_AUTOUIC ON)
 #file(GLOB_RECURSE UI_FILES "./ui/*.ui")
 #message(UI_FILES: ${UI_FILES})
 #QT5_WRAP_UI(FORMS_UIC ${UI_FILES})

 #message(FORMS_UIC: ${FORMS_UIC})
 
 #FILE(GLOB_RECURSE HEADER_FILES "*.h")
 #QT5_WRAP_CPP(MOC_FILES ${HEADER_FILES})
 #message(MOC_FILES: ${MOC_FILES})
 include_directories("E:/git_pro/vcpkg/installed/x86-windows/include")
link_directories("E:/git_pro/vcpkg/installed/x86-windows/lib")
SET(SRC_FILES 

"BindWidget.cpp"
"EditImage.cpp"
"EditImg.cpp"
"get_color_dlg.cpp"
"MainWindow.cpp"
"main.cpp"
"Mbutton.hpp"
"systemfont_dlg.cpp"
"Tool.cpp"
"cap_dialog.h"
"../libop/imageProc/imageProc.cpp"
"../libop/imageProc/imageLoc.cpp"
"../libop/core/globalVar.cpp"
"../libop/core/helpfunc.cpp"
"../libop/core/pipe.cpp"
"../libop/core/opEnv.cpp"
)


find_package(Qt5Core)
find_package(Qt5Widgets)
find_package(Qt5Gui)
find_package(Qt5OpenGL)
find_package(Qt5Xml)



add_executable(qttool ${SRC_FILES} Tool.qrc)


Set_Target_Properties(qttool PROPERTIES LINK_FLAGS_RELEASE "/SUBSYSTEM:WINDOWS /ENTRY:mainCRTStartup")

#target_link_libraries(qttool PRIVATE Qt5::Widgets)
target_link_libraries(qttool ScintillaEdit4.lib)
target_link_libraries(qttool lua)
qt5_use_modules(qttool Core Widgets  Gui)

IF(CMAKE_CL_64)
  install(TARGETS qttool  DESTINATION "${PROJECT_SOURCE_DIR}/bin/x64")
ELSE(CMAKE_CL_64)
  install(TARGETS qttool  DESTINATION "${PROJECT_SOURCE_DIR}/bin/x86")
ENDIF(CMAKE_CL_64)