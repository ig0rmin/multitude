#ifndef LUMINOUS_RENDER_TARGET_HPP
#define LUMINOUS_RENDER_TARGET_HPP

#include "Export.hpp"
#include "RenderResource.hpp"
#include "Texture2.hpp"

#include <Nimble/Vector2.hpp>

#include <QSize>

namespace Luminous
{

  class LUMINOUS_API RenderBuffer : public RenderResource
  {
  public:
    RenderBuffer();
    ~RenderBuffer();

    RenderBuffer(RenderBuffer && rb);
    RenderBuffer & operator=(RenderBuffer && rb);

    void storageFormat(const QSize & size, GLenum format, int samples);

    const QSize & size() const;
    GLenum format() const;
    int samples() const;

  private:
    class D;
    D * m_d;
  };

  ////////////////////////////////////////////////////////////////////////////////
  ////////////////////////////////////////////////////////////////////////////////

  class LUMINOUS_API RenderTarget : public RenderResource
  {
  public:
    enum RenderTargetType
    {
        INVALID
      , WINDOW
      , NORMAL
    };

    RenderTarget(RenderTargetType type = NORMAL);
    ~RenderTarget();

    RenderTarget(RenderTarget && rt);
    RenderTarget & operator=(RenderTarget && rt);

    const QSize & size() const;
    void setSize(const QSize & size);

    void attach(GLenum attachment, Luminous::Texture & texture);
    void attach(GLenum attachment, Luminous::RenderBuffer & buffer);

    Luminous::Texture * texture(GLenum attachment) const;
    Luminous::RenderBuffer * renderBuffer(GLenum attachment) const;

    QList<GLenum> textureAttachments() const;
    QList<GLenum> renderBufferAttachments() const;

    RenderTargetType targetType() const;

  private:
    class D;
    D * m_d;
  };

  ////////////////////////////////////////////////////////////////////////////////
  ////////////////////////////////////////////////////////////////////////////////

  class RenderContext;

  class LUMINOUS_API RenderTargetGuard
  {
  public:
    RenderTargetGuard(RenderContext & r);
    ~RenderTargetGuard();

  private:
    RenderContext & m_renderContext;
  };
}

#endif
