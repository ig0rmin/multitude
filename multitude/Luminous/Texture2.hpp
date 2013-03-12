/* COPYRIGHT
 *
 * This file is part of Luminous.
 *
 * Copyright: MultiTouch Oy, Helsinki University of Technology and others, 2007-2013
 *
 * This file is licensed under GNU Lesser General Public
 * License (LGPL), version 2.1. The LGPL conditions can be found in 
 * file "LGPL.txt" that is distributed with this source package or obtained 
 * from the GNU organization (www.gnu.org).
 * 
 */

#if !defined (LUMINOUS_TEXTURE2_HPP)
#define LUMINOUS_TEXTURE2_HPP

#include "Luminous/Luminous.hpp"
#include "Luminous/RenderResource.hpp"

#include <Radiant/Color.hpp>

#include <QRegion>

namespace Luminous
{

  /// A GPU texture. This class contains the necessary CPU-side information
  /// about a texture. This class does not own the memory it handles. All
  /// pointers given to this class must remain valid during the lifetime of
  /// this object.
  /// GPU correspondent of this class is TextureGL
  class Texture : public RenderResource
  {
  public:
    /// Texture filter mode
    enum Filter {
      /// Point-sampled filtering
      FILTER_NEAREST = GL_NEAREST,
      /// Linear filtering
      FILTER_LINEAR = GL_LINEAR
    };

    /// Texture wrap mode
    enum Wrap {
      /// Repeat texture
      WRAP_REPEAT,
      /// Mirror texture
      WRAP_MIRROR,
      /// Clamp texture
      WRAP_CLAMP,
      /// Clamp-to-border
      WRAP_BORDER
    };

  public:
    /// Construct a new texture
    LUMINOUS_API Texture();
    /// Destructor
    LUMINOUS_API ~Texture();

    /// Construct a copy
    /// @param tex texture to copy
    LUMINOUS_API Texture(Texture & tex);
    /// Copy a texture
    /// @param tex texture to copy
    LUMINOUS_API Texture & operator=(Texture & tex);

    /// Construct texture by moving
    /// @param tex texture to move
    LUMINOUS_API Texture(Texture && tex);
    /// Move a texture
    /// @param tex texture to move
    LUMINOUS_API Texture & operator=(Texture && tex);

    /// Specify the number of color components in the texture. Use 0 to let
    /// Cornerstone select the default format. See "internalFormat" parameter
    /// description in
    /// http://www.opengl.org/sdk/docs/man3/xhtml/glTexImage2D.xml
    /// @param format number of color components
    LUMINOUS_API void setInternalFormat(int format);
    /// Get the number of color components in the texture
    /// @return number of color components
    LUMINOUS_API int internalFormat() const;

    /// Set 1D texture data from memory. This function assumes 1D data and will automatically set the texture height to 1.
    /// @param width width of the texture
    /// @param dataFormat data format
    /// @param data pointer to data
    LUMINOUS_API void setData(unsigned int width, const PixelFormat & dataFormat, const void * data);
    /// Set 2D texture data from memory.
    /// @param width width of the texture
    /// @param height height of the texture
    /// @param dataFormat data format
    /// @param data data pointer
    LUMINOUS_API void setData(unsigned int width, unsigned int height, const PixelFormat & dataFormat, const void * data);
    /// Set 3D texture data from memory.
    /// @param width width of the texture
    /// @param height height of the texture
    /// @param depth depth of the texture
    /// @param dataFormat data format
    /// @param data data pointer
    LUMINOUS_API void setData(unsigned int width, unsigned int height, unsigned int depth, const PixelFormat & dataFormat, const void * data);

    /// Reset the texture to invalid state
    LUMINOUS_API void reset();

    /// Get the size of the texture data in bytes
    /// @return texture size in bytes
    LUMINOUS_API std::size_t dataSize() const;

    /// Set the texture line size in pixels. Use 0 to use width of the texture.
    /// @param size size of one line in pixels
    LUMINOUS_API void setLineSizePixels(std::size_t size);
    /// Get the texture line size in pixels
    /// @return line size in pixels
    LUMINOUS_API unsigned int lineSizePixels() const;

    /// Check if the texture is valid. Texture is considered valid, if its dimension has been defined.
    /// @return true if the texture is valid; otherwise false
    /// @sa dimensions
    LUMINOUS_API bool isValid() const;

    /// Get texture dimension. 1D textures will have dimension 1, 2D texture 2 and 3D textures 3.
    /// @return texture dimension
    LUMINOUS_API uint8_t dimensions() const;
    /// Get the width of the texture
    /// @return texture width in pixels
    LUMINOUS_API unsigned int width() const;
    /// Get the height of the texture
    /// @return texture height in pixels
    LUMINOUS_API unsigned int height() const;
    /// Get the depth of the texture
    /// @return texture depth in pixels
    LUMINOUS_API unsigned int depth() const;
    /// Get the pixel format of the texture data
    /// @return data format
    LUMINOUS_API const PixelFormat & dataFormat() const;
    /// Get the raw pointer to texture data.
    /// @return pointer to texture data
    LUMINOUS_API const void * data() const;

    /// Mark a region of the texture as dirty. Dirty regions are used to
    /// determine which parts of the texture data must be uploaded to GPU
    /// memory. Use this function if you want to optimize texture uploads to
    /// GPU. For example, if you only update a small region of a large texture,
    /// you can just mark the changed region as dirty and Cornerstone will only
    /// upload that part of the texture to the GPU.
    /// @param rect dirty region define in pixel coordinates
    LUMINOUS_API void addDirtyRect(const QRect & rect);

    /// Get the texture dirty region for the given thread. This functions
    /// returns the dirty region of the texture for the given thread. This
    /// function is used by Cornerstone internally. There should be no reason
    /// to call this function manually.
    /// @param threadIndex index of the thread to query the region for
    /// @return dirty region for the given thread
    LUMINOUS_API QRegion dirtyRegion(unsigned int threadIndex) const;

    /// Get the dirty region for the given thread and clear the dirty region.
    /// This functions returns the dirty region of the texture and
    /// simultaneously clears it to empty. This function is used by Cornerstone
    /// internally and there should be no reason to call this function
    /// manually.
    /// @param threadIndex index of the thread to query
    /// @return dirty region for the given thread
    LUMINOUS_API QRegion takeDirtyRegion(unsigned int threadIndex) const;

    /// Get the sample count of the texture. By default, the sample count is
    /// zero for textures that are not multi-sampled.
    /// @return sample count
    LUMINOUS_API unsigned int samples() const;

    /// Set the number of samples for the texture. This function can be used to
    /// define multi-sampled textures.
    /// @param samples sample count
    LUMINOUS_API void setSamples(unsigned int samples);

    /// Check if the texture is translucent.
    /// @sa setTranslucency
    LUMINOUS_API bool translucent() const;

    /// Set translucency flag to the texture. This flag is used by Cornerstone
    /// to optimize rendering. Rendering opaque textures can be sorted for
    /// optimal performance. Rendering translucent textures can not be sorted.
    /// A texture must be marked as translucent if it has an alpha channel and
    /// any pixel in the texture has an alpha component other than one. Failure
    /// to do so may cause rendering artifacts as incorrect blending.
    /// @param translucency texture translucency
    /// @sa translucent
    LUMINOUS_API void setTranslucency(bool translucency);

    /// Get the texture minification filter mode
    /// @return filtering used for texture minification
    LUMINOUS_API Filter getMinFilter() const;
    /// Set texture minification filtering
    /// @param filter filtering used
    LUMINOUS_API void setMinFilter(Filter filter);

    /// Get texture magnification filter mode
    /// @return filtering used for texture magnification
    LUMINOUS_API Filter getMagFilter() const;
    /// Set texture magnification filtering
    /// @param filter filtering used
    LUMINOUS_API void setMagFilter(Filter filter);

    /// Set the texture wrap mode
    /// @param s wrap mode for s texture coordinate
    /// @param t wrap mode for t texture coordinate
    /// @param r wrap mode for r texture coordinate
    LUMINOUS_API void setWrap(Wrap s, Wrap t, Wrap r);
    /// Get the texture wrap mode
    /// @param[out] s wrap mode for s coordinate
    /// @param[out] t wrap mode for t coordinate
    /// @param[out] r wrap mode for r coordinate
    LUMINOUS_API void getWrap(Wrap & s, Wrap & t, Wrap & r) const;

    /// Set the texture border color
    /// @param color border color
    LUMINOUS_API void setBorderColor(const Radiant::Color & color);
    /// Get the texture border color
    /// @return border color
    LUMINOUS_API const Radiant::Color & borderColor() const;

  private:
    class D;
    D * m_d;
  };
}
#endif // LUMINOUS_TEXTURE2_HPP
