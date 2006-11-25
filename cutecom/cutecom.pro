SOURCES	+= main.cpp qcppdialogimpl.cpp
HEADERS	+= qcppdialogimpl.h
unix {
  UI_DIR = ./
  MOC_DIR = .moc
  OBJECTS_DIR = .obj
}

FORMS	= cutecommdlg.ui
TEMPLATE	=app
CONFIG	+= qt warn_on release thread
LANGUAGE	= C++

DESKTOP_PATH = $$system(kde-config --install apps --expandvars)

desktop.files=cutecom.desktop
desktop.path=$$DESKTOP_PATH/Utilities
#desktop.extra=echo "CuteCom will be installed in the Utilities submenu"

target.path = /usr/local/bin
INSTALLS += target desktop

