/* COPYRIGHT
 *
 * This file is part of Luminous.
 *
 * Copyright: MultiTouch Oy, Helsinki University of Technology and others.
 *
 * See file "Luminous.hpp" for authors and more details.
 *
 * This file is licensed under GNU Lesser General Public
 * License (LGPL), version 2.1. The LGPL conditions can be found in 
 * file "LGPL.txt" that is distributed with this source package or obtained 
 * from the GNU organization (www.gnu.org).
 * 
 */

#ifndef LUMINOUS_LUMINOUS_HPP
#define LUMINOUS_LUMINOUS_HPP

#include <Luminous/Export.hpp>
#include <Radiant/Platform.hpp>
#include <Radiant/Trace.hpp>

#ifdef RADIANT_OSX
// We do not use GLEW on OSX
#define MULTI_WITHOUT_GLEW 1
#endif

#ifdef MULTI_WITHOUT_GLEW
// This define is for glext.h
# define GL_GLEXT_PROTOTYPES 1

# ifdef RADIANT_OSX
#  ifdef RADIANT_IOS
//#   include <OpenGLES/ES1/gl.h>
#   include <OpenGLES/ES2/gl.h>
#   ifdef LUMINOUS_COMPILE
//#    include <OpenGLES/ES1/glext.h>
#    include <OpenGLES/ES2/glext.h>
#   endif
#   define LUMINOUS_OPENGLES 1
#  else
#   include <OpenGL/gl.h>
#   include <OpenGL/glext.h>
#   include <OpenGL/glu.h>
#  endif
# else
#  define GL_GLEXT_PROTOTYPES 1
#  include <GL/gl.h>
#  include <GL/glext.h>
#  include <GL/glu.h>
# endif
#else
# include <GL/glew.h>
# if defined (RADIANT_WINDOWS)
#   include <GL/wglew.h>
# endif
#endif

#ifdef LUMINOUS_OPENGLES
# define LUMINOUS_IN_FULL_OPENGL(x)
#else
# define LUMINOUS_OPENGL_FULL
# define LUMINOUS_IN_FULL_OPENGL(x) x
#endif


#ifdef LUMINOUS_OPENGLES
# include <Luminous/DummyOpenGL.hpp>

# define glGenRenderbuffersEXT glGenRenderbuffers
# define glDeleteRenderbuffersEXT glDeleteRenderbuffers
# define glBindRenderbufferEXT glBindRenderbuffer
# define glRenderbufferStorageEXT glRenderbufferStorage
# define GL_RENDERBUFFER_EXT GL_RENDERBUFFER

# define glGenFramebuffersEXT glGenFramebuffers
# define glDeleteFramebuffersEXT glDeleteFramebuffers
# define glBindFramebufferEXT glBindFramebuffer
# define glCheckFramebufferStatusEXT glCheckFramebufferStatus
# define glFramebufferRenderbufferEXT glFramebufferRenderbuffer
# define glFramebufferRenderbufferEXT glFramebufferRenderbuffer
# define glFramebufferTexture2DEXT glFramebufferTexture2D

# define GL_FRAMEBUFFER_EXT GL_FRAMEBUFFER
# define GL_FRAMEBUFFER_COMPLETE_EXT GL_FRAMEBUFFER_COMPLETE
# define GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT_EXT GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT
# define GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT_EXT GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT
# define GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS_EXT GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS
# define GL_FRAMEBUFFER_INCOMPLETE_FORMATS_EXT GL_FRAMEBUFFER_INCOMPLETE_FORMATS
# define GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER_EXT GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER
# define GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER_EXT GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER
# define GL_FRAMEBUFFER_UNSUPPORTED_EXT GL_FRAMEBUFFER_UNSUPPORTED

#endif

#define debugLuminous(...) (Radiant::trace("Luminous", Radiant::DEBUG, __VA_ARGS__))
/// Luminous is a library of C++ classes for computer graphics, using OpenGL.
/** Copyright: The Luminous library has been developed in Helsinki
    Institute for Information Technology (HIIT, 2006-2008) and
    MultiTouch Oy (2007-2008).

    Luminous is released under the GNU Lesser General Public License
    (LGPL), version 2.1.
*/
namespace Luminous
{

  /** Initializes the Luminous library.
      In practice this function only initializes the GLEW and checks
      the capabilities of the underlying OpenGL implementation. If the
      OpenGL version is below 2.0, then a warning message is
      issued.

      @param initOpenGL if set to false, glew will not be initialized

      @return true if all relevant resources were successfully
      initialized, false if something was left missing (for example
      too low OpenGL version).
  */
  LUMINOUS_API bool initLuminous(bool initOpenGL = true);

  /** Initializes the default image codecs.
  The image codecs are loaded as plugins that need to be loaded before they can
  be used. This functions does just that.
  */
  LUMINOUS_API void initDefaultImageCodecs();


  //////////////////////////////////////////////////////////////////////////
  enum PrimitiveType
  {
    PT_Triangle,
    PT_TriangleStrip,
    PT_Line,
    PT_LineStrip,
    PT_Point,
  };

  /// @note Only the write options are supported on OpenGL/ES
  enum BufferUsage {
    BU_Unknown,        // Undefined
    // Defined once, used many times
    BU_Static_Read,    // GPU->CPU
    BU_Static_Write,   // CPU->GPU
    BU_Static_Copy,    // GPU->GPU
    // Defined repeatedly, used many times
    BU_Dynamic_Read,   // GPU->CPU
    BU_Dynamic_Write,  // CPU->GPU
    BU_Dynamic_Copy,   // GPU->GPU
    // Defined repeatedly, used a few times
    BU_Stream_Read,    // GPU->CPU 
    BU_Stream_Write,   // CPU->GPU
    BU_Stream_Copy,    // GPU->GPU
  };

  enum BufferType {
    BT_VertexBuffer,
    BT_IndexBuffer,
    BT_ConstantBuffer,
  };

  enum DataType
  {
    DT_Unknown,
    DT_Byte,
    DT_Short,
    DT_Int,
    DT_UnsignedByte,
    DT_UnsignedShort,
    DT_UnsignedInt,
    DT_Float,
    DT_Double
  };
  
  enum BufferLockOptions {
    BLO_Discard     = (1 << 0),
    BLO_Read        = (1 << 1),
    BLO_Write       = (1 << 2),
    BLO_NoOverwrite = (1 << 3),
    BLO_ReadWrite   = BLO_Read | BLO_Write,
  };

  enum ClearMask
  {
    CM_Color              = (1 << 0),
    CM_Depth              = (1 << 1),
    CM_Stencil            = (1 << 2),
    CM_ColorDepth         = CM_Color | CM_Depth,
    CM_ColorStencil       = CM_Color | CM_Stencil,
    CM_DepthStencil       = CM_Depth | CM_Stencil,
    CM_ColorDepthStencil  = CM_Color | CM_Depth | CM_Stencil,
  };

  enum ShaderType
  {
    ST_VertexShader,
    ST_FragmentShader,
    ST_GeometryShader,
  };

  enum BlendFunction
  {

  };
  enum BlendEquation
  {

  };

  enum RenderBin
  {
    RB_Opaque,
    RB_Transparent,
  };

  enum ResourceType {
    RT_Unknown,
    RT_VertexArray,
    RT_ShaderProgram,
    RT_VertexShader,
    RT_FragmentShader,
    RT_GeometryShader,
    RT_Texture,
    RT_Buffer,
  };

  //////////////////////////////////////////////////////////////////////////
  // Utility functions
  /// @todo Luminous2 utilities, should rename once Luminous::Utils has been killed with fire
  namespace Utils2
  {
    size_t getDataSize(DataType type);
  }

  //////////////////////////////////////////////////////////////////////////
  /// Forward declarations

  // 
  class RenderDriver;
  class GLContext;
  // Resources
  class HardwareBuffer;
  class ShaderConstantBlock;
  class ShaderProgram;
  class ShaderGLSL;
  // Vertex data
  struct VertexAttribute;
  class VertexDescription;
  class VertexAttributeBinding;
}

#endif
