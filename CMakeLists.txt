cmake_minimum_required(VERSION 2.8.11 FATAL_ERROR )

project(CuteCom)

set (CuteCom_MAJOR_VERSION 0)
set (CuteCom_MINOR_VERSION 25)
set (CuteCom_PATCH_VERSION 0)
set (CuteCom_VERSION
     ${CuteCom_MAJOR_VERSION}.${CuteCom_MINOR_VERSION}.${CuteCom_PATCH_VERSION})

configure_file (
  "${PROJECT_SOURCE_DIR}/version.h.in"
  "${PROJECT_SOURCE_DIR}/version.h"
)

# Find includes in corresponding build directories
set(CMAKE_INCLUDE_CURRENT_DIR ON)
# Instruct CMake to run moc automatically when needed.
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

# Find the Qt libraries
find_package(Qt5Widgets REQUIRED)
find_package(Qt5Core REQUIRED)
find_package(Qt5Gui REQUIRED)
#find_package(Serialport REQUIRED)

qt5_wrap_ui(uiHeaders controlpanel.ui  mainwindow.ui statusbar.ui sessionmanager.ui)
set(cutecomSrcs main.cpp mainwindow.cpp controlpanel.cpp  devicecombo.cpp
    serialdevicelistmodel.cpp  settings.cpp statusbar.cpp sessionmanager.cpp
    datadisplay.cpp)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
# C++14: set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++1y")
#add_definitions("-std=c++11")

set(exeType)
set(binInstallDir bin )

if(APPLE)
   set(exeType MACOSX_BUNDLE)
   set(binInstallDir /Applications )
endif(APPLE)

add_executable(cutecom ${exeType} ${cutecomSrcs} ${uiHeaders} resources.qrc)


qt5_use_modules(cutecom Core Gui Widgets SerialPort)

if (APPLE)
   set_target_properties(cutecom PROPERTIES OUTPUT_NAME CuteCom)
endif (APPLE)

install(TARGETS cutecom DESTINATION ${binInstallDir} )

if(CMAKE_COMPILER_IS_GNUCC OR CMAKE_COMPILER_IS_GNUCXX)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wno-long-long -pedantic")
endif()

set (CPACK_PACKAGE_VERSION ${CuteCom_VERSION})
set (CPACK_SOURCE_GENERATOR "TBZ2")
set (CPACK_GENERATOR "RPM")
set (CPACK_PACKAGE_NAME "cutecom")
set (CPACK_PACKAGE_RELEASE 1)
set (CPACK_PACKAGE_VENDOR "cyc1insir")
set (CPACK_PACKAGE_INSTALL_PREFIX ${CMAKE_INSTALL_PREFIX})
set (CPACK_PACKAGE_FILE_NAME "${CPACK_PACKAGE_NAME}-${CPACK_PACKAGE_VERSION}-${CPACK_PACKAGE_RELEASE}.${CMAKE_SYSTEM_PROCESSOR}")
configure_file (
    "${CMAKE_CURRENT_SOURCE_DIR}/distribution/rpm-generic.spec.in" 
    "${CMAKE_CURRENT_BINARY_DIR}/cutecom.spec"
    @ONLY IMMEDIATE )
set (CPACK_RPM_USER_BINARY_SPECFILE "${CMAKE_CURRENT_BINARY_DIR}/cutecom.spec")
set (CPACK_SOURCE_PACKAGE_FILE_NAME 
       "${CPACK_PACKAGE_NAME}-${CPACK_PACKAGE_VERSION}")
set (CPACK_SOURCE_IGNORE_FILES
       "${CMAKE_CURRENT_BINARY_DIR};/.git/;~$;${CPACK_SOURCE_IGNORE_FILES}")
include (CPack)

add_custom_target(dist COMMAND git archive --format tar --prefix=cutecom-${CuteCom_VERSION}/ HEAD | bzip2 > ${CMAKE_CURRENT_BINARY_DIR}/${CPACK_SOURCE_PACKAGE_FILE_NAME}.tar.bz2 WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})

