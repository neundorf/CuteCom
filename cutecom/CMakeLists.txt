cmake_minimum_required(VERSION 2.4.3 FATAL_ERROR )

project(CuteCom)

find_package(Qt4 REQUIRED)

include_directories(${QT_INCLUDES} ${CMAKE_BINARY_DIR})
add_definitions(-DQT3_SUPPORT)
set(cutecomSrcs main.cpp qcppdialogimpl.cpp)

qt4_wrap_cpp(cutecomSrcs qcppdialogimpl.h)

qt4_wrap_ui(cutecomSrcs cutecommdlg.ui)

add_executable(cutecom ${cutecomSrcs} ${uiHeaders})
target_link_libraries(cutecom ${QT_QT3SUPPORT_LIBRARY} ${QT_QTGUI_LIBRARY} ${QT_QTCORE_LIBRARY})

install(TARGETS cutecom DESTINATION bin )

install(FILES cutecom.1 DESTINATION man/man1 )

find_program(KDECONFIG_EXECUTABLE NAMES kde-config )

if (KDECONFIG_EXECUTABLE)
   # then ask kde-config for the kde data dirs
   exec_program(${KDECONFIG_EXECUTABLE} ARGS --install apps --expandvars OUTPUT_VARIABLE _apps_DIR )
   install(FILES cutecom.desktop DESTINATION ${_apps_DIR}/Utilities)
endif (KDECONFIG_EXECUTABLE)
