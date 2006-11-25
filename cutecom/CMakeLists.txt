cmake_minimum_required(VERSION 2.4.3 FATAL_ERROR )

find_package(Qt3 REQUIRED)

include_directories(${QT_INCLUDE_DIR} ${CMAKE_BINARY_DIR})

set(cutecomSrcs main.cpp qcppdialogimpl.cpp)

qt_wrap_cpp(foo cutecomSrcs qcppdialogimpl.h)

qt_wrap_ui(foo uiHeaders cutecomSrcs cutecommdlg.ui)

add_executable(cutecom ${cutecomSrcs} ${uiHeaders})
target_link_libraries(cutecom ${QT_LIBRARIES})

install(TARGETS cutecom DESTINATION bin )



find_program(KDECONFIG_EXECUTABLE NAMES kde-config )

if (KDECONFIG_EXECUTABLE)
   # then ask kde-config for the kde data dirs
   exec_program(${KDECONFIG_EXECUTABLE} ARGS --install apps --expandvars OUTPUT_VARIABLE _apps_DIR )
   install(FILES cutecom.desktop DESTINATION ${_apps_DIR}/Utilities)
endif (KDECONFIG_EXECUTABLE)
