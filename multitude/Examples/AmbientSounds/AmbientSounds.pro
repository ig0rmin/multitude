include(../Examples.pri)
SOURCES += Main.cpp
LIBS += $$LIB_RADIANT \
    $$LIB_RESONANT \
    $$LIB_VALUABLE \
    $$LIB_PATTERNS
win32 {
    INCLUDEPATH += $$INC_WINPORT
    LIBPATH += $$LNK_MULTITUDE
    LIBS += ws2_32.lib
}
HEADERS += ../../../Tests/CameraWidget/VideoAnnotations.hpp
