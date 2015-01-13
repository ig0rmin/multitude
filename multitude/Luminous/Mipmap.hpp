/* Copyright (C) 2007-2013: Multi Touch Oy, Helsinki University of Technology
 * and others.
 *
 * This file is licensed under GNU Lesser General Public License (LGPL),
 * version 2.1. The LGPL conditions can be found in file "LGPL.txt" that is
 * distributed with this source package or obtained from the GNU organization
 * (www.gnu.org).
 * 
 */

#ifndef LUMINOUS_MIPMAP_HPP
#define LUMINOUS_MIPMAP_HPP

#include "Luminous.hpp"

#include "Radiant/Task.hpp"

#include <Nimble/Vector2.hpp>
#include <Nimble/Matrix4.hpp>

#include <Valuable/Node.hpp>
#include <Valuable/State.hpp>

#include <QString>
#include <QFuture>

namespace Luminous
{

  /// This class provides a custom mipmap management for images loaded from
  /// disk.
  class Mipmap : public Valuable::Node,
                 public std::enable_shared_from_this<Mipmap>
  {
  public:
    LUMINOUS_API ~Mipmap();

    /// Gets the texture mipmap.
    /// @param level The mipmap level
    /// @return Pointer to the texture or null if image if mipmap level isn't loaded yet
    LUMINOUS_API Texture * texture(unsigned int level = 0, unsigned int * returnedLevel = nullptr,
                                   int priorityChange = 0);

    LUMINOUS_API Image * image(unsigned int level = 0, unsigned int * returnedLevel = nullptr,
                               int priorityChange = 0);

#ifndef LUMINOUS_OPENGLES
    /** Gets the compressed image on given level.
        @param level the mipmap level
        @return pointer to the mipmap */
    LUMINOUS_API CompressedImage * compressedImage(unsigned int level = 0, unsigned int * returnedLevel = nullptr,
                                                   int priorityChange = 0);
#endif

    /// Calculate the ideal mipmap level
    LUMINOUS_API unsigned int level(const Nimble::Matrix4 & transform, Nimble::SizeF pixelSize,
                                    float * trilinearBlending = nullptr) const;
    LUMINOUS_API unsigned int level(Nimble::SizeF pixelSize,
                                    float * trilinearBlending = nullptr) const;

    /** @return Returns the native size of the image, in pixels. */
    LUMINOUS_API const Nimble::Size & nativeSize() const;
    /// Returns the aspect ratio of the image in its native size (width/height)
    /** If the native height is zero (e.g. no file was loaded) this returns 1.*/
    LUMINOUS_API float aspect() const;

    /// Mipmap is not "header-ready", if it still has PingTask running/waiting
    /// After the mipmap is header-ready, nativeSize() returns the correct size
    LUMINOUS_API bool isHeaderReady() const;

    /// Mipmap is not ready, if it still has PingTask or MipmapGeneratorTask running/waiting
    LUMINOUS_API bool isReady() const;

    LUMINOUS_API bool isValid() const;

    /// @return Returns true if the images have alpha channel
    LUMINOUS_API bool hasAlpha() const;

    /// Reads alpha value in the most accurate loaded mipmap level in given coordinate
    /// This function doesn't trigger any mipmap loading
    /// @param relLoc relative XY-location, values should be from 0 to 1
    /// @return alpha value from 0 to 1, or 1 if reading fails
    LUMINOUS_API float pixelAlpha(Nimble::Vector2 relLoc) const;

    /// Sets the loading priority for this set of mipmaps
    /// @param priority new priority
    LUMINOUS_API void setLoadingPriority(Radiant::Priority priority);

    /// Returns the size of the mipmap level
    LUMINOUS_API Nimble::Size mipmapSize(unsigned int level);

    /// Returns the absolute filename of the image.
    /// @return filename from which the mipmaps have been created
    LUMINOUS_API const QString & filename() const;

    LUMINOUS_API Radiant::TaskPtr pingTask();
    LUMINOUS_API Radiant::TaskPtr mipmapGeneratorTask();
    LUMINOUS_API Radiant::TaskPtr loadingTask();

    LUMINOUS_API Valuable::LoadingState & state();
    LUMINOUS_API const Valuable::LoadingState & state() const;

    /// Gets a shared pointer to an image file CPU-side mipmap.
    /// @param filename The name of the image file
    /// @param compressedMipmaps control whether compressed mipmaps should be used
    LUMINOUS_API static std::shared_ptr<Mipmap> acquire(const QString & filename,
                                                        bool compressedMipmaps);

    /// Returns cache filename for given source file name.
    /// @param src The original image filename
    /// @param level Mipmap level, ignored if negative
    /// @param suffix File format of the cache file name, usually png or dds.
    /// @return cache filename
    LUMINOUS_API static QString cacheFileName(const QString & src, int level = -1,
                                              const QString & suffix = "png");

    /// Returns path to dir that contains all cached mipmaps
    LUMINOUS_API static QString imageCachePath();

  private:
    Mipmap(const QString & filenameAbs);

    void setMipmapReady(const ImageInfo & imginfo);
    void startLoading(bool compressedMipmaps);

  private:
    friend class PingTask;
    friend class LoadImageTask;
    friend class MipmapReleaseTask;
    class D;
    D * m_d;
  };

  /// Shared pointer to Mipmap
  typedef std::shared_ptr<Mipmap> MipmapPtr;

}

#endif // LUMINOUS_MIPMAP_HPP
