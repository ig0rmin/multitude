include(../Applications.pri)

SOURCES += Main.cpp

LIBS += $$LIB_RADIANT $$LIB_NIMBLE
LIBS += $$LIB_PATTERNS $$LIB_VALUABLE $$LIB_V8

unix: PKGCONFIG += portaudio-2.0
CONFIG -= qt

win* {
	INCLUDEPATH += ../../Win64x/include/portaudio
	INCLUDEPATH += ../../Win64x/include/libsndfile

  QMAKE_LIBDIR += $$DDK_PATH\\lib\\win7\\amd64
  LIBS += -llibsndfile-1 -lportaudio_x64 -lole32 -luser32
}

include(../Applications_end.pri)
