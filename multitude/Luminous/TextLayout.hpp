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

#ifndef LUMINOUS_TEXTLAYOUT_HPP
#define LUMINOUS_TEXTLAYOUT_HPP

#include <Valuable/Node.hpp>

#include <Luminous/RenderCommand.hpp>

#include <Nimble/Rect.hpp>

#include <array>
#include <vector>

class QGlyphRun;

namespace Luminous {
  /// TextLayout is the base class for different implementations of text layouting.
  /// It is a node since it sends layout-event
  /// @event[out] layout New layout has been made - boundingBox and other
  ///                    things might have changed.
  class TextLayout : public Valuable::Node
  {
  public:
    /// The bounds for a single glyph in the layout
    struct LUMINOUS_API Item
    {
      std::array<FontVertex, 4> vertices;
    };

  public:
    LUMINOUS_API virtual ~TextLayout();

    LUMINOUS_API TextLayout(TextLayout && t);
    LUMINOUS_API TextLayout & operator=(TextLayout && t);

    LUMINOUS_API int groupCount() const;
    LUMINOUS_API Texture * texture(int groupIndex) const;
    LUMINOUS_API const std::vector<Item> & items(int groupIndex) const;

    LUMINOUS_API bool isLayoutReady() const;
    LUMINOUS_API bool isComplete() const;
    /// Not thread safe
    LUMINOUS_API void generate();

    LUMINOUS_API bool correctAtlas() const;

    LUMINOUS_API void invalidate();

    LUMINOUS_API void check() const;

    LUMINOUS_API virtual void setMaximumSize(const Nimble::Vector2f & size);
    LUMINOUS_API Nimble::Vector2f maximumSize() const;
    /// Returns the bounding box of the text. Not thread safe.
    LUMINOUS_API const Nimble::Rectf & boundingBox() const;

    LUMINOUS_API const Nimble::Vector2f & renderLocation() const;

  protected:
    LUMINOUS_API virtual void generateInternal() const = 0;

    LUMINOUS_API TextLayout(const Nimble::Vector2f & maximumSize);

    LUMINOUS_API void setRenderLocation(const Nimble::Vector2f & location);
    LUMINOUS_API void setBoundingBox(const Nimble::Rectf & bb);
    LUMINOUS_API void setLayoutReady(bool v);
    LUMINOUS_API void setGlyphsReady(bool v);
    LUMINOUS_API void clearGlyphs();
    LUMINOUS_API bool generateGlyphs(const Nimble::Vector2f & location,
                                     const QGlyphRun & glyphRun);

  private:
    class D;
    D * m_d;
  };
} // namespace Luminous

#endif // LUMINOUS_TEXTLAYOUT_HPP
