/* Copyright (C) 2007-2013: Multi Touch Oy, Helsinki University of Technology
 * and others.
 *
 * This file is licensed under GNU Lesser General Public License (LGPL),
 * version 2.1. The LGPL conditions can be found in file "LGPL.txt" that is
 * distributed with this source package or obtained from the GNU organization
 * (www.gnu.org).
 * 
 */

#ifndef LUMINOUS_RENDER_TARGET_HPP
#define LUMINOUS_RENDER_TARGET_HPP

#include "Export.hpp"
#include "RenderResource.hpp"
#include "Texture2.hpp"

#include <Nimble/Vector2.hpp>
#include <Nimble/Size.hpp>

namespace Luminous
{


  /// This class represents an off-screen render target that is optimized for
  /// use as a render target. This class should be used if you do not need to
  /// sample (e.g. use as a texture) your rendered image.
  class LUMINOUS_API RenderBuffer : public RenderResource
  {
  public:
    /// Construct a new RenderBuffer
    RenderBuffer();
    /// Destructor
    ~RenderBuffer();

    /// Construct a copy of the given RenderBuffer
    /// @param rb buffer to copy
    RenderBuffer(RenderBuffer & rb);
    /// Copy the given RenderBuffer
    /// @param rb buffer to copy
    RenderBuffer & operator=(RenderBuffer & rb);

    /// Move constructor
    /// @param rb buffer to move
    RenderBuffer(RenderBuffer && rb);
    /// Move the given RenderBuffer
    /// @param rb buffer to move
    RenderBuffer & operator=(RenderBuffer && rb);

    /// Set the data storage, format, dimensions and sample count of the
    /// RenderBuffer's buffer
    /// @param size dimensions of the buffer
    /// @param format data format
    /// @param samples buffer sample count
    void setStorageFormat(const Nimble::Size &size, GLenum format, int samples);

    /// Get the dimensions of the buffer
    /// @return dimensions of the buffer
    const Nimble::Size & size() const;
    /// Get the buffer format
    /// @return format of the buffer
    GLenum format() const;
    /// Get the sample count of the buffer
    /// @return buffer sample count
    int samples() const;

  private:
    class D;
    D * m_d;
  };

  ////////////////////////////////////////////////////////////////////////////////
  ////////////////////////////////////////////////////////////////////////////////

  /// This class is an abstraction of a generic render target. It provides an
  /// abstraction of the OpenGL FrameBufferObject API.
  /// @todo document, add API to detach attachments, rename to framebuffer
  class LUMINOUS_API RenderTarget
      : public RenderResource, public Patterns::NotCopyable
  {
  private:
    class D;
    D * m_d;

    /// This class is a helper used to implement copying RenderTarget classes.
    /// You should never manually instantiate this class. It is also meant to be
    /// used with RenderTarget::deepCopy, RenderTarget::shallowCopy, and
    /// RenderTarget::shallowCopyNoAttachments functions.
    /// @sa RenderTarget
    class LUMINOUS_API RenderTargetCopy
    {
    private:
      RenderTargetCopy(D * d) : m_d(d) {}

      D * m_d;

      friend class RenderTarget;
    };

    ////////////////////////////////////////////////////////////////////////////////
    ////////////////////////////////////////////////////////////////////////////////

  public:
    enum RenderTargetType
    {
        INVALID
      , WINDOW
      , NORMAL
    };

    enum RenderTargetBind
    {
        BIND_DEFAULT
      , BIND_READ
      , BIND_DRAW
    };

    RenderTarget(RenderTargetType type = NORMAL);
    ~RenderTarget();

    RenderTarget(const RenderTargetCopy & rt);
    RenderTarget & operator=(const RenderTargetCopy & rt);

    RenderTarget(RenderTarget && rt);
    RenderTarget & operator=(RenderTarget && rt);

    RenderTargetCopy shallowCopyNoAttachments() const;
    RenderTargetCopy shallowCopy() const;
    RenderTargetCopy deepCopy() const;

    const Nimble::Size & size() const;
    void setSize(const Nimble::Size &size);

    unsigned samples() const;
    void setSamples(unsigned int samples);

    void attach(GLenum attachment, Luminous::Texture & texture);
    void attach(GLenum attachment, Luminous::RenderBuffer & buffer);

    Luminous::Texture & createTextureAttachment(GLenum attachment, const Luminous::PixelFormat & format);
    Luminous::RenderBuffer & createRenderBufferAttachment(GLenum attachment, GLenum storageFormat);

    Luminous::Texture * texture(GLenum attachment) const;
    Luminous::RenderBuffer * renderBuffer(GLenum attachment) const;

    QList<GLenum> textureAttachments() const;
    QList<GLenum> renderBufferAttachments() const;

    RenderTargetType targetType() const;

    RenderTargetBind targetBind() const;
    void setTargetBind(RenderTargetBind bind);
  };

  ////////////////////////////////////////////////////////////////////////////////
  ////////////////////////////////////////////////////////////////////////////////

  class RenderContext;

  /// This class is an utility class that automatically pops a render target
  /// from the given RenderContext when destroyed.
  class LUMINOUS_API RenderTargetGuard
  {
  public:
    /// Construct a new guard
    /// @param r render context to pop a target from
    RenderTargetGuard(RenderContext & r);
    ~RenderTargetGuard();

  private:
    RenderContext & m_renderContext;
  };
}

#endif
