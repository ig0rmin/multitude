# Construct path to dependency package
CORNERSTONE_FULL_VERSION_STR = $$cat(../../CORNERSTONE_VERSION)

CORNERSTONE_VERSION = $$section(CORNERSTONE_FULL_VERSION_STR, "-", 0, 0)
CORNERSTONE_VERSION_MAJOR = $$section(CORNERSTONE_VERSION, ".", 0, 0)
CORNERSTONE_VERSION_MINOR = $$section(CORNERSTONE_VERSION, ".", 1, 1)
CORNERSTONE_VERSION_PATCH = $$section(CORNERSTONE_VERSION, ".", 2, 2)
CORNERSTONE_SHORT_VERSION_STR = $${CORNERSTONE_VERSION_MAJOR}.$${CORNERSTONE_VERSION_MINOR}

CORNERSTONE_DEPS_PATH=C:/Cornerstone-$${CORNERSTONE_SHORT_VERSION_STR}-deps

# Install Windows 3rd party dlls to bin
win64_runtime_dlls.path = /bin
win64_runtime_dlls.files = $$PWD/bin64/*
win64_runtime_dlls.files += $$CORNERSTONE_DEPS_PATH/ssl/*dll

# Install libraries under 'lib'
win64_sdk_libs2.path = /lib
win64_sdk_libs2.files = $$PWD/lib64/*

# Install headers under 'include'
win64_sdk_headers2.path = /include
win64_sdk_headers2.files = $$PWD/include/*

win64_libav_bins.path = /bin

win64_libav_libs1.path = /lib

win64_libav_headers1.path = /include

win64_libav_bins.files = $$CORNERSTONE_DEPS_PATH/ffmpeg/bin/*dll
win64_libav_bins.files += $$CORNERSTONE_DEPS_PATH/ffmpeg/bin/*exe

win64_libav_libs1.files = $$CORNERSTONE_DEPS_PATH/ffmpeg/bin/*lib

win64_libav_headers1.files = $$CORNERSTONE_DEPS_PATH/ffmpeg/include/*

INSTALLS += win64_libav_bins
INSTALLS += win64_libav_libs1
INSTALLS += win64_libav_headers1

win64_ghostscript.path = /bin
win64_ghostscript.files = $$CORNERSTONE_DEPS_PATH/ghostscript/*

INSTALLS += win64_ghostscript

win64_node_dlls1.path = /bin
win64_node_dlls1.files = $$CORNERSTONE_DEPS_PATH/node/bin/node.exe
win64_node_dlls1.files += $$CORNERSTONE_DEPS_PATH/node/bin/*.cmd
win64_node_dlls1.files += $$CORNERSTONE_DEPS_PATH/node/bin/*.dll
win64_node_dlls1.files += $$CORNERSTONE_DEPS_PATH/node/bin/node_modules

win64_node_libs1.path = /lib
win64_node_libs1.files = $$CORNERSTONE_DEPS_PATH/node/lib/*lib

win64_node_headers1.path = /include
win64_node_headers1.files = $$CORNERSTONE_DEPS_PATH/node/include/*

win64_argyll.path = /bin
win64_argyll.files = $$CORNERSTONE_DEPS_PATH/argyll/spotread.exe

win64_ruby.path = /tools
win64_ruby.files = $$CORNERSTONE_DEPS_PATH/ruby

win64_clang_headers.path = /include/libcxx
win64_clang_headers.files = $$CORNERSTONE_DEPS_PATH/libcxx/include/*

win64_clangxml.path = /bin
win64_clangxml.files = $$CORNERSTONE_DEPS_PATH/clangxml/ClangXML.exe

win64_boost_headers1.path = /include
win64_boost_headers1.files = $$CORNERSTONE_DEPS_PATH/boost_1_55_0/boost

win64_curl_dlls1.path = /bin
win64_curl_dlls1.files = $$CORNERSTONE_DEPS_PATH/curl/dlls/*

win64_curl_libs1.path = /lib
win64_curl_libs1.files = $$CORNERSTONE_DEPS_PATH/curl/lib/*

win64_curl_headers1.path = /include
win64_curl_headers1.files = $$CORNERSTONE_DEPS_PATH/curl/include/*

INSTALLS += win64_node_dlls1
INSTALLS += win64_node_libs1
INSTALLS += win64_node_headers1
INSTALLS += win64_ruby
INSTALLS += win64_clang_headers
INSTALLS += win64_clangxml

INSTALLS += win64_argyll
INSTALLS += win64_runtime_dlls
INSTALLS += win64_sdk_libs2
INSTALLS += win64_sdk_headers2
INSTALLS += win64_curl_dlls1
INSTALLS += win64_curl_libs1
INSTALLS += win64_curl_headers1
INSTALLS += win64_boost_headers1

# Install Qt
qt_bin_files.path = /bin
qt_bin_files.files = $$[QT_INSTALL_BINS]/*.exe
qt_bin_files.files = $$[QT_INSTALL_BINS]/*.dll

qt_conf_files.path = /bin
qt_conf_files.files = $$PWD/qt.conf

qt_lib_files.path = /qt/lib
qt_lib_files.files = $$[QT_INSTALL_LIBS]\\*.lib

qt_files.path = /qt
qt_files.files += $$[QT_INSTALL_IMPORTS]
qt_files.files += $$[QT_INSTALL_TRANSLATIONS]
qt_files.files += $$[QMAKE_MKSPECS]
qt_files.files += $$[QT_INSTALL_HEADERS]

qt_plugins.path = /qt
qt_plugins.extra = $$QMAKE_COPY_DIR $$shell_path($$[QT_INSTALL_PLUGINS]/*.dll) $(INSTALL_ROOT)\qt\plugins

INSTALLS += qt_bin_files qt_lib_files qt_files qt_conf_files qt_plugins

# Install Cef
win64_cef_dlls.path = /bin
win64_cef_dlls.files = $$CORNERSTONE_DEPS_PATH/cef/Release/*

win64_cef_resources.path = /Resources
win64_cef_resources.files = $$CORNERSTONE_DEPS_PATH/cef/Resources/*

# Crashpad
crashpad_files.path = /bin
crashpad_files.files = $$CORNERSTONE_DEPS_PATH/crashpad/bin/*

INSTALLS += win64_cef_dlls win64_cef_resources crashpad_files

message(Including 64-bit Windows Libraries)
