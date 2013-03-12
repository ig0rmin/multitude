/* Copyright (C) 2007-2013: Multi Touch Oy, Helsinki University of Technology
 * and others.
 *
 * This file is licensed under GNU Lesser General Public License (LGPL),
 * version 2.1. The LGPL conditions can be found in file "LGPL.txt" that is
 * distributed with this source package or obtained from the GNU organization
 * (www.gnu.org).
 * 
 */

#include "Error.hpp"

#include <Radiant/Trace.hpp>
#include <Radiant/Mutex.hpp>

#include <QMap>

// OS X (Core 3.2) doesn't have these
#if defined(RADIANT_OSX)
#  define GL_STACK_OVERFLOW 0x0503
#  define GL_STACK_UNDERFLOW 0x504
#  define GL_TABLE_TOO_LARGE 0x8031
#endif

namespace Luminous
{

  void glErrorToString(const QString & msg, int line)
  {
    static QMap<GLuint, QString> errors;

    MULTI_ONCE {

      errors.insert(GL_NO_ERROR, "no error");
      errors.insert(GL_INVALID_ENUM, "invalid enumerant");
      errors.insert(GL_INVALID_VALUE, "invalid value");
      errors.insert(GL_INVALID_OPERATION, "invalid operation");
      errors.insert(GL_STACK_OVERFLOW, "stack overflow");
      errors.insert(GL_STACK_UNDERFLOW, "stack underflow");
      errors.insert(GL_OUT_OF_MEMORY, "out of memory");
      errors.insert(GL_TABLE_TOO_LARGE, "table too large");
      errors.insert(GL_INVALID_FRAMEBUFFER_OPERATION, "invalid framebuffer operation");

    }

    GLenum err, err2 = GL_NO_ERROR;
    while((err = glGetError()) != GL_NO_ERROR) {
      // If glGetError ever returns the same error twice, it's broken somehow.
      // This happens when called without GL context etc.
      if (err == err2) {
        Radiant::error("%s # glGetError called with broken OpenGL context", msg.toUtf8().data());
        return;
      }
      err2 = err;
      Radiant::error("%s:%d: %s", msg.toUtf8().data(), line, errors.value(err).toUtf8().data());
    };
  }
}
