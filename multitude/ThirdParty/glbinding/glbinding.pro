######################################################################
# Automatically generated by qmake (3.0) Thu Dec 31 14:59:19 2015
######################################################################
include(../../multitude.pri)

TEMPLATE = lib
CONFIG += staticlib
TARGET = glbinding
INCLUDEPATH += . include

win32 {
  # Use multi-byte character set instead of unicode
  DEFINES -= UNICODE
  DEFINES += _MBCS

  # Work around C1128
  QMAKE_CXXFLAGS += /bigobj
}

# Build static library
DEFINES += GLBINDING_STATIC_DEFINE


# Input
HEADERS += source/Binding_pch.h \
           source/callbacks_private.h \
           source/glrevision.h \
           source/logging_private.h \
           source/Meta_Maps.h \
           source/RingBuffer.h \
           source/RingBuffer.hpp \
           include/glbinding/AbstractFunction.h \
           include/glbinding/AbstractValue.h \
           include/glbinding/Binding.h \
           include/glbinding/callbacks.h \
           include/glbinding/ContextHandle.h \
           include/glbinding/ContextInfo.h \
           include/glbinding/Function.h \
           include/glbinding/Function.hpp \
           include/glbinding/glbinding_api.h \
           include/glbinding/glbinding_features.h \
           include/glbinding/logging.h \
           include/glbinding/Meta.h \
           include/glbinding/nogl.h \
           include/glbinding/ProcAddress.h \
           include/glbinding/SharedBitfield.h \
           include/glbinding/SharedBitfield.hpp \
           include/glbinding/Value.h \
           include/glbinding/Value.hpp \
           include/glbinding/Version.h \
           include/glbinding/gl/bitfield.h \
           include/glbinding/gl/boolean.h \
           include/glbinding/gl/enum.h \
           include/glbinding/gl/extension.h \
           include/glbinding/gl/functions.h \
           include/glbinding/gl/gl.h \
           include/glbinding/gl/types.h \
           include/glbinding/gl/values.h \
           include/glbinding/gl10/bitfield.h \
           include/glbinding/gl10/boolean.h \
           include/glbinding/gl10/enum.h \
           include/glbinding/gl10/functions.h \
           include/glbinding/gl10/gl.h \
           include/glbinding/gl10/types.h \
           include/glbinding/gl10/values.h \
           include/glbinding/gl10ext/bitfield.h \
           include/glbinding/gl10ext/boolean.h \
           include/glbinding/gl10ext/enum.h \
           include/glbinding/gl10ext/functions.h \
           include/glbinding/gl10ext/gl.h \
           include/glbinding/gl10ext/types.h \
           include/glbinding/gl10ext/values.h \
           include/glbinding/gl11/bitfield.h \
           include/glbinding/gl11/boolean.h \
           include/glbinding/gl11/enum.h \
           include/glbinding/gl11/functions.h \
           include/glbinding/gl11/gl.h \
           include/glbinding/gl11/types.h \
           include/glbinding/gl11/values.h \
           include/glbinding/gl11ext/bitfield.h \
           include/glbinding/gl11ext/boolean.h \
           include/glbinding/gl11ext/enum.h \
           include/glbinding/gl11ext/functions.h \
           include/glbinding/gl11ext/gl.h \
           include/glbinding/gl11ext/types.h \
           include/glbinding/gl11ext/values.h \
           include/glbinding/gl12/bitfield.h \
           include/glbinding/gl12/boolean.h \
           include/glbinding/gl12/enum.h \
           include/glbinding/gl12/functions.h \
           include/glbinding/gl12/gl.h \
           include/glbinding/gl12/types.h \
           include/glbinding/gl12/values.h \
           include/glbinding/gl12ext/bitfield.h \
           include/glbinding/gl12ext/boolean.h \
           include/glbinding/gl12ext/enum.h \
           include/glbinding/gl12ext/functions.h \
           include/glbinding/gl12ext/gl.h \
           include/glbinding/gl12ext/types.h \
           include/glbinding/gl12ext/values.h \
           include/glbinding/gl13/bitfield.h \
           include/glbinding/gl13/boolean.h \
           include/glbinding/gl13/enum.h \
           include/glbinding/gl13/functions.h \
           include/glbinding/gl13/gl.h \
           include/glbinding/gl13/types.h \
           include/glbinding/gl13/values.h \
           include/glbinding/gl13ext/bitfield.h \
           include/glbinding/gl13ext/boolean.h \
           include/glbinding/gl13ext/enum.h \
           include/glbinding/gl13ext/functions.h \
           include/glbinding/gl13ext/gl.h \
           include/glbinding/gl13ext/types.h \
           include/glbinding/gl13ext/values.h \
           include/glbinding/gl14/bitfield.h \
           include/glbinding/gl14/boolean.h \
           include/glbinding/gl14/enum.h \
           include/glbinding/gl14/functions.h \
           include/glbinding/gl14/gl.h \
           include/glbinding/gl14/types.h \
           include/glbinding/gl14/values.h \
           include/glbinding/gl14ext/bitfield.h \
           include/glbinding/gl14ext/boolean.h \
           include/glbinding/gl14ext/enum.h \
           include/glbinding/gl14ext/functions.h \
           include/glbinding/gl14ext/gl.h \
           include/glbinding/gl14ext/types.h \
           include/glbinding/gl14ext/values.h \
           include/glbinding/gl15/bitfield.h \
           include/glbinding/gl15/boolean.h \
           include/glbinding/gl15/enum.h \
           include/glbinding/gl15/functions.h \
           include/glbinding/gl15/gl.h \
           include/glbinding/gl15/types.h \
           include/glbinding/gl15/values.h \
           include/glbinding/gl15ext/bitfield.h \
           include/glbinding/gl15ext/boolean.h \
           include/glbinding/gl15ext/enum.h \
           include/glbinding/gl15ext/functions.h \
           include/glbinding/gl15ext/gl.h \
           include/glbinding/gl15ext/types.h \
           include/glbinding/gl15ext/values.h \
           include/glbinding/gl20/bitfield.h \
           include/glbinding/gl20/boolean.h \
           include/glbinding/gl20/enum.h \
           include/glbinding/gl20/functions.h \
           include/glbinding/gl20/gl.h \
           include/glbinding/gl20/types.h \
           include/glbinding/gl20/values.h \
           include/glbinding/gl20ext/bitfield.h \
           include/glbinding/gl20ext/boolean.h \
           include/glbinding/gl20ext/enum.h \
           include/glbinding/gl20ext/functions.h \
           include/glbinding/gl20ext/gl.h \
           include/glbinding/gl20ext/types.h \
           include/glbinding/gl20ext/values.h \
           include/glbinding/gl21/bitfield.h \
           include/glbinding/gl21/boolean.h \
           include/glbinding/gl21/enum.h \
           include/glbinding/gl21/functions.h \
           include/glbinding/gl21/gl.h \
           include/glbinding/gl21/types.h \
           include/glbinding/gl21/values.h \
           include/glbinding/gl21ext/bitfield.h \
           include/glbinding/gl21ext/boolean.h \
           include/glbinding/gl21ext/enum.h \
           include/glbinding/gl21ext/functions.h \
           include/glbinding/gl21ext/gl.h \
           include/glbinding/gl21ext/types.h \
           include/glbinding/gl21ext/values.h \
           include/glbinding/gl30/bitfield.h \
           include/glbinding/gl30/boolean.h \
           include/glbinding/gl30/enum.h \
           include/glbinding/gl30/functions.h \
           include/glbinding/gl30/gl.h \
           include/glbinding/gl30/types.h \
           include/glbinding/gl30/values.h \
           include/glbinding/gl30ext/bitfield.h \
           include/glbinding/gl30ext/boolean.h \
           include/glbinding/gl30ext/enum.h \
           include/glbinding/gl30ext/functions.h \
           include/glbinding/gl30ext/gl.h \
           include/glbinding/gl30ext/types.h \
           include/glbinding/gl30ext/values.h \
           include/glbinding/gl31/bitfield.h \
           include/glbinding/gl31/boolean.h \
           include/glbinding/gl31/enum.h \
           include/glbinding/gl31/functions.h \
           include/glbinding/gl31/gl.h \
           include/glbinding/gl31/types.h \
           include/glbinding/gl31/values.h \
           include/glbinding/gl31ext/bitfield.h \
           include/glbinding/gl31ext/boolean.h \
           include/glbinding/gl31ext/enum.h \
           include/glbinding/gl31ext/functions.h \
           include/glbinding/gl31ext/gl.h \
           include/glbinding/gl31ext/types.h \
           include/glbinding/gl31ext/values.h \
           include/glbinding/gl32/bitfield.h \
           include/glbinding/gl32/boolean.h \
           include/glbinding/gl32/enum.h \
           include/glbinding/gl32/functions.h \
           include/glbinding/gl32/gl.h \
           include/glbinding/gl32/types.h \
           include/glbinding/gl32/values.h \
           include/glbinding/gl32core/bitfield.h \
           include/glbinding/gl32core/boolean.h \
           include/glbinding/gl32core/enum.h \
           include/glbinding/gl32core/functions.h \
           include/glbinding/gl32core/gl.h \
           include/glbinding/gl32core/types.h \
           include/glbinding/gl32core/values.h \
           include/glbinding/gl32ext/bitfield.h \
           include/glbinding/gl32ext/boolean.h \
           include/glbinding/gl32ext/enum.h \
           include/glbinding/gl32ext/functions.h \
           include/glbinding/gl32ext/gl.h \
           include/glbinding/gl32ext/types.h \
           include/glbinding/gl32ext/values.h \
           include/glbinding/gl33/bitfield.h \
           include/glbinding/gl33/boolean.h \
           include/glbinding/gl33/enum.h \
           include/glbinding/gl33/functions.h \
           include/glbinding/gl33/gl.h \
           include/glbinding/gl33/types.h \
           include/glbinding/gl33/values.h \
           include/glbinding/gl33core/bitfield.h \
           include/glbinding/gl33core/boolean.h \
           include/glbinding/gl33core/enum.h \
           include/glbinding/gl33core/functions.h \
           include/glbinding/gl33core/gl.h \
           include/glbinding/gl33core/types.h \
           include/glbinding/gl33core/values.h \
           include/glbinding/gl33ext/bitfield.h \
           include/glbinding/gl33ext/boolean.h \
           include/glbinding/gl33ext/enum.h \
           include/glbinding/gl33ext/functions.h \
           include/glbinding/gl33ext/gl.h \
           include/glbinding/gl33ext/types.h \
           include/glbinding/gl33ext/values.h \
           include/glbinding/gl40/bitfield.h \
           include/glbinding/gl40/boolean.h \
           include/glbinding/gl40/enum.h \
           include/glbinding/gl40/functions.h \
           include/glbinding/gl40/gl.h \
           include/glbinding/gl40/types.h \
           include/glbinding/gl40/values.h \
           include/glbinding/gl40core/bitfield.h \
           include/glbinding/gl40core/boolean.h \
           include/glbinding/gl40core/enum.h \
           include/glbinding/gl40core/functions.h \
           include/glbinding/gl40core/gl.h \
           include/glbinding/gl40core/types.h \
           include/glbinding/gl40core/values.h \
           include/glbinding/gl40ext/bitfield.h \
           include/glbinding/gl40ext/boolean.h \
           include/glbinding/gl40ext/enum.h \
           include/glbinding/gl40ext/functions.h \
           include/glbinding/gl40ext/gl.h \
           include/glbinding/gl40ext/types.h \
           include/glbinding/gl40ext/values.h \
           include/glbinding/gl41/bitfield.h \
           include/glbinding/gl41/boolean.h \
           include/glbinding/gl41/enum.h \
           include/glbinding/gl41/functions.h \
           include/glbinding/gl41/gl.h \
           include/glbinding/gl41/types.h \
           include/glbinding/gl41/values.h \
           include/glbinding/gl41core/bitfield.h \
           include/glbinding/gl41core/boolean.h \
           include/glbinding/gl41core/enum.h \
           include/glbinding/gl41core/functions.h \
           include/glbinding/gl41core/gl.h \
           include/glbinding/gl41core/types.h \
           include/glbinding/gl41core/values.h \
           include/glbinding/gl41ext/bitfield.h \
           include/glbinding/gl41ext/boolean.h \
           include/glbinding/gl41ext/enum.h \
           include/glbinding/gl41ext/functions.h \
           include/glbinding/gl41ext/gl.h \
           include/glbinding/gl41ext/types.h \
           include/glbinding/gl41ext/values.h \
           include/glbinding/gl42/bitfield.h \
           include/glbinding/gl42/boolean.h \
           include/glbinding/gl42/enum.h \
           include/glbinding/gl42/functions.h \
           include/glbinding/gl42/gl.h \
           include/glbinding/gl42/types.h \
           include/glbinding/gl42/values.h \
           include/glbinding/gl42core/bitfield.h \
           include/glbinding/gl42core/boolean.h \
           include/glbinding/gl42core/enum.h \
           include/glbinding/gl42core/functions.h \
           include/glbinding/gl42core/gl.h \
           include/glbinding/gl42core/types.h \
           include/glbinding/gl42core/values.h \
           include/glbinding/gl42ext/bitfield.h \
           include/glbinding/gl42ext/boolean.h \
           include/glbinding/gl42ext/enum.h \
           include/glbinding/gl42ext/functions.h \
           include/glbinding/gl42ext/gl.h \
           include/glbinding/gl42ext/types.h \
           include/glbinding/gl42ext/values.h \
           include/glbinding/gl43/bitfield.h \
           include/glbinding/gl43/boolean.h \
           include/glbinding/gl43/enum.h \
           include/glbinding/gl43/functions.h \
           include/glbinding/gl43/gl.h \
           include/glbinding/gl43/types.h \
           include/glbinding/gl43/values.h \
           include/glbinding/gl43core/bitfield.h \
           include/glbinding/gl43core/boolean.h \
           include/glbinding/gl43core/enum.h \
           include/glbinding/gl43core/functions.h \
           include/glbinding/gl43core/gl.h \
           include/glbinding/gl43core/types.h \
           include/glbinding/gl43core/values.h \
           include/glbinding/gl43ext/bitfield.h \
           include/glbinding/gl43ext/boolean.h \
           include/glbinding/gl43ext/enum.h \
           include/glbinding/gl43ext/functions.h \
           include/glbinding/gl43ext/gl.h \
           include/glbinding/gl43ext/types.h \
           include/glbinding/gl43ext/values.h \
           include/glbinding/gl44/bitfield.h \
           include/glbinding/gl44/boolean.h \
           include/glbinding/gl44/enum.h \
           include/glbinding/gl44/functions.h \
           include/glbinding/gl44/gl.h \
           include/glbinding/gl44/types.h \
           include/glbinding/gl44/values.h \
           include/glbinding/gl44core/bitfield.h \
           include/glbinding/gl44core/boolean.h \
           include/glbinding/gl44core/enum.h \
           include/glbinding/gl44core/functions.h \
           include/glbinding/gl44core/gl.h \
           include/glbinding/gl44core/types.h \
           include/glbinding/gl44core/values.h \
           include/glbinding/gl44ext/bitfield.h \
           include/glbinding/gl44ext/boolean.h \
           include/glbinding/gl44ext/enum.h \
           include/glbinding/gl44ext/functions.h \
           include/glbinding/gl44ext/gl.h \
           include/glbinding/gl44ext/types.h \
           include/glbinding/gl44ext/values.h \
           include/glbinding/gl45/bitfield.h \
           include/glbinding/gl45/boolean.h \
           include/glbinding/gl45/enum.h \
           include/glbinding/gl45/functions.h \
           include/glbinding/gl45/gl.h \
           include/glbinding/gl45/types.h \
           include/glbinding/gl45/values.h \
           include/glbinding/gl45core/bitfield.h \
           include/glbinding/gl45core/boolean.h \
           include/glbinding/gl45core/enum.h \
           include/glbinding/gl45core/functions.h \
           include/glbinding/gl45core/gl.h \
           include/glbinding/gl45core/types.h \
           include/glbinding/gl45core/values.h \
           include/glbinding/gl45ext/bitfield.h \
           include/glbinding/gl45ext/boolean.h \
           include/glbinding/gl45ext/enum.h \
           include/glbinding/gl45ext/functions.h \
           include/glbinding/gl45ext/gl.h \
           include/glbinding/gl45ext/types.h \
           include/glbinding/gl45ext/values.h
SOURCES += source/AbstractFunction.cpp \
           source/AbstractValue.cpp \
           source/Binding.cpp \
           source/Binding_list.cpp \
           source/Binding_objects.cpp \
           source/Binding_objects_a.cpp \
           source/Binding_objects_b.cpp \
           source/Binding_objects_c.cpp \
           source/Binding_objects_d.cpp \
           source/Binding_objects_e.cpp \
           source/Binding_objects_f.cpp \
           source/Binding_objects_g.cpp \
           source/Binding_objects_h.cpp \
           source/Binding_objects_i.cpp \
           source/Binding_objects_j.cpp \
           source/Binding_objects_k.cpp \
           source/Binding_objects_l.cpp \
           source/Binding_objects_m.cpp \
           source/Binding_objects_n.cpp \
           source/Binding_objects_o.cpp \
           source/Binding_objects_p.cpp \
           source/Binding_objects_q.cpp \
           source/Binding_objects_r.cpp \
           source/Binding_objects_s.cpp \
           source/Binding_objects_t.cpp \
           source/Binding_objects_u.cpp \
           source/Binding_objects_v.cpp \
           source/Binding_objects_w.cpp \
           source/Binding_objects_x.cpp \
           source/Binding_objects_y.cpp \
           source/Binding_objects_z.cpp \
           source/Binding_pch.cpp \
           source/callbacks.cpp \
           source/ContextHandle.cpp \
           source/ContextInfo.cpp \
           source/logging.cpp \
           source/Meta.cpp \
           source/Meta_BitfieldsByString.cpp \
           source/Meta_BooleansByString.cpp \
           source/Meta_EnumsByString.cpp \
           source/Meta_ExtensionsByFunctionString.cpp \
           source/Meta_ExtensionsByString.cpp \
           source/Meta_FunctionStringsByExtension.cpp \
           source/Meta_getStringByBitfield.cpp \
           source/Meta_ReqVersionsByExtension.cpp \
           source/Meta_StringsByBitfield.cpp \
           source/Meta_StringsByBoolean.cpp \
           source/Meta_StringsByEnum.cpp \
           source/Meta_StringsByExtension.cpp \
           source/ProcAddress.cpp \
           source/Value.cpp \
           source/Version.cpp \
           source/Version_ValidVersions.cpp \
           source/gl/functions.cpp \
           source/gl/functions_a.cpp \
           source/gl/functions_b.cpp \
           source/gl/functions_c.cpp \
           source/gl/functions_d.cpp \
           source/gl/functions_e.cpp \
           source/gl/functions_f.cpp \
           source/gl/functions_g.cpp \
           source/gl/functions_h.cpp \
           source/gl/functions_i.cpp \
           source/gl/functions_j.cpp \
           source/gl/functions_k.cpp \
           source/gl/functions_l.cpp \
           source/gl/functions_m.cpp \
           source/gl/functions_n.cpp \
           source/gl/functions_o.cpp \
           source/gl/functions_p.cpp \
           source/gl/functions_q.cpp \
           source/gl/functions_r.cpp \
           source/gl/functions_s.cpp \
           source/gl/functions_t.cpp \
           source/gl/functions_u.cpp \
           source/gl/functions_v.cpp \
           source/gl/functions_w.cpp \
           source/gl/functions_x.cpp \
           source/gl/functions_y.cpp \
           source/gl/functions_z.cpp \
           source/gl/types.cpp

DESTDIR = ../../lib

include(../../library.pri)
