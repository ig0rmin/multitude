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

#ifndef LUMINOUS_SIMPLE_TEXT_LAYOUT_HPP
#define LUMINOUS_SIMPLE_TEXT_LAYOUT_HPP

#include <Valuable/Valuable.hpp>
#include "TextLayout.hpp"

class QTextOption;
class QFont;
class QTextLayout;

namespace Luminous
{
  /// Plain text, usually one font, inside rectangle (0,0) -> size
  class SimpleTextLayout : public TextLayout
  {
  public:
    LUMINOUS_API SimpleTextLayout();
    LUMINOUS_API SimpleTextLayout(const SimpleTextLayout & that);
    LUMINOUS_API SimpleTextLayout(const QString & text, const Nimble::Vector2f & maximumSize,
                                  const QFont & font, const QTextOption & textOption);
    LUMINOUS_API virtual ~SimpleTextLayout();

    LUMINOUS_API void setText(const QString & text);

    LUMINOUS_API QTextOption textOption() const;
    LUMINOUS_API void setTextOption(const QTextOption & textOption);

    LUMINOUS_API QFont font() const;
    LUMINOUS_API void setFont(const QFont & font);

    LUMINOUS_API void setLineHeight(const Valuable::StyleValue & height);
    LUMINOUS_API const Valuable::StyleValue & lineHeight() const;

    LUMINOUS_API void setLetterSpacing(const Valuable::StyleValue & spacing);
    LUMINOUS_API const Valuable::StyleValue & letterSpacing() const;

    /// If the QTextLayout is modified, it's required to call invalidate() manually
    LUMINOUS_API QTextLayout & layout();
    LUMINOUS_API const QTextLayout & layout() const;

    LUMINOUS_API static const SimpleTextLayout & cachedLayout(const QString & text,
                                                              const Nimble::Vector2f & size,
                                                              const QFont & font,
                                                              const QTextOption & option);

    LUMINOUS_API virtual void generateInternal() const OVERRIDE;

  private:
    class D;
    D * m_d;
  };
} // namespace Luminous

#endif // LUMINOUS_SIMPLE_TEXT_LAYOUT_HPP
