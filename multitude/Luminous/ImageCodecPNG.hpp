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

#ifndef CODEC_PNG_HPP
#define CODEC_PNG_HPP

/// @cond

#include <Luminous/ImageCodec.hpp>

namespace Luminous
{

  /// Image codec to handle PNG files
  class ImageCodecPNG : public ImageCodec
  {
    public:
      virtual ~ImageCodecPNG();
      virtual bool canRead(FILE * file);

      virtual QString extensions() const;
      virtual QString name() const;
      virtual bool ping(ImageInfo & info, FILE * file);
      virtual bool read(Image & image, FILE * file);
      virtual bool write(const Image & image, FILE * file);
  };

}

/// @endcond

#endif
