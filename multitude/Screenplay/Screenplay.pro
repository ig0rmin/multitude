include(../multitude.pri)

HEADERS += Export.hpp
HEADERS += VideoFFMPEG.hpp
HEADERS += ScreenPlay.hpp

SOURCES += VideoFFMPEG.cpp

LIBS += $$MULTI_FFMPEG_LIBS

LIBS += $$LIB_RADIANT $$LIB_PATTERNS

DEFINES += SCREENPLAY_EXPORT	

win32 {
	win64:INCLUDEPATH += ../Win64x/include/ffmpeg
	else:INCLUDEPATH += ../Win32x/include/ffmpeg
}


include(../library.pri)
