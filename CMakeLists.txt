cmake_minimum_required(VERSION 2.8.11 FATAL_ERROR )

project(CuteCom)

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

qt5_wrap_ui(uiHeaders controlpanel.ui  mainwindow.ui statusbar.ui)
set(cutecomSrcs main.cpp mainwindow.cpp controlpanel.cpp  devicecombo.cpp serialdevicelistmodel.cpp  settings.cpp statusbar.cpp)

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


