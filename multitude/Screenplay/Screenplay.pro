include(../multitude.pri)

HEADERS += Export.hpp
HEADERS += VideoFFMPEG.hpp
HEADERS += ScreenPlay.hpp

SOURCES += VideoFFMPEG.cpp

LIBS += $$MULTI_FFMPEG_LIBS
#LIBS += avcodec.lib avutil.lib avformat.lib

LIBS += $$LIB_RADIANT $$LIB_PATTERNS

win32 {
	DEFINES += SCREENPLAY_EXPORT	
	
	win64:INCLUDEPATH += ../Win64x/include/ffmpeg
	else:INCLUDEPATH += ../Win32x/include/ffmpeg
}


include(../library.pri)
