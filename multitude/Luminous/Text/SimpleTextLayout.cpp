#include "SimpleTextLayout.hpp"

#include <Radiant/Mutex.hpp>

#include <Valuable/StyleValue.hpp>

#include <QFontMetricsF>
#include <QTextLayout>

#include <unordered_map>
#include <memory>
#include <tuple>

namespace std
{
  template<> struct hash<std::tuple<QString, Nimble::Vector2i, QFont, QTextOption>>
  {
    inline size_t operator()(const std::tuple<QString, Nimble::Vector2i, QFont, QTextOption> & tuple) const
    {
      std::hash<uint> hasher;
      return qHash(std::get<0>(tuple)) ^ hasher(std::get<1>(tuple).x) ^
          hasher(std::get<1>(tuple).y) ^ qHash(std::get<2>(tuple).key()) ^
          hasher(std::get<3>(tuple).alignment());
    }
  };
}

bool operator==(const QTextOption & o1, const QTextOption & o2)
{
  return int(o1.alignment()) == int(o2.alignment()) &&
      o1.flags() == o2.flags() &&
      o1.tabStop() == o2.tabStop() &&
      o1.tabs() == o2.tabs() &&
      o1.textDirection() == o2.textDirection() &&
      o1.useDesignMetrics() == o2.useDesignMetrics() &&
      o1.wrapMode() == o2.wrapMode();
}

namespace
{
  std::unordered_map<std::tuple<QString, Nimble::Vector2i, QFont, QTextOption>, std::unique_ptr<Luminous::SimpleTextLayout>> s_layoutCache;
  Radiant::Mutex s_layoutCacheMutex;
}

namespace Luminous
{
  class SimpleTextLayout::D
  {
  public:
    D(const QString & text, const QFont & font, const QTextOption & option);

    void layout(const Nimble::Vector2f & size);

  public:
    Valuable::StyleValue m_lineHeight;
    QTextLayout m_layout;
  };

  /////////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////////

  SimpleTextLayout::D::D(const QString & text, const QFont & font,
                         const QTextOption & option)
    : m_layout(text, font)
  {
    m_layout.setTextOption(option);
  }

  void SimpleTextLayout::D::layout(const Nimble::Vector2f & size)
  {
    assert(m_layout.font().hintingPreference() == QFont::PreferNoHinting);
    QFontMetricsF fontMetrics(m_layout.font());
    const float lineWidth = size.x;
    const float leading = fontMetrics.leading();

    bool forceHeight = false;
    float height = 0.0f;
    float heightFactor = 1.0f;
    if (m_lineHeight.size() == 1) {
      if (m_lineHeight.unit() == Valuable::Attribute::VU_PXS) {
        forceHeight = true;
        height = m_lineHeight.asFloat();
      } else if (m_lineHeight.unit() == Valuable::Attribute::VU_UNKNOWN ||
                 m_lineHeight.unit() == Valuable::Attribute::VU_PERCENTAGE) {
        heightFactor = m_lineHeight.asFloat();
      }
    }

    float y = 0;
    m_layout.beginLayout();
    while (true) {
      QTextLine line = m_layout.createLine();
      if (!line.isValid())
        break;

      line.setLineWidth(lineWidth);
      y += leading;
      line.setPosition(QPointF(0, y));
      if (forceHeight)
        y += height;
      else
        y += line.height() * heightFactor;
    }
    m_layout.endLayout();
  }

  /////////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////////

  SimpleTextLayout::SimpleTextLayout()
    : TextLayout(Nimble::Vector2f(100, 100))
  {
    QFont f;
    f.setHintingPreference(QFont::PreferNoHinting);
    m_d = new D(QString(), f, QTextOption());
  }

  SimpleTextLayout::SimpleTextLayout(const QString & text, const Nimble::Vector2f & maximumSize,
                                     const QFont & font, const QTextOption & textOption)
    : TextLayout(maximumSize)
  {
    QFont f(font);
    f.setHintingPreference(QFont::PreferNoHinting);
    m_d = new D(text, f, textOption);
  }

  SimpleTextLayout::~SimpleTextLayout()
  {
    delete m_d;
  }

  void SimpleTextLayout::setLineHeight(const Valuable::StyleValue & height)
  {
    m_d->m_lineHeight = height;
    setLayoutReady(false);
  }

  const Valuable::StyleValue & SimpleTextLayout::lineHeight() const
  {
    return m_d->m_lineHeight;
  }

  QTextLayout & SimpleTextLayout::layout()
  {
    return m_d->m_layout;
  }

  const QTextLayout & SimpleTextLayout::layout() const
  {
    return m_d->m_layout;
  }

  const SimpleTextLayout & SimpleTextLayout::cachedLayout(const QString & text,
                                                          const Nimble::Vector2f & size,
                                                          const QFont & font,
                                                          const QTextOption & option)
  {
    SimpleTextLayout * layout;

    {
      /// @todo someone should also delete old layouts..
      Radiant::Guard g(s_layoutCacheMutex);
      std::unique_ptr<SimpleTextLayout> & ptr = s_layoutCache[std::make_tuple(text, size.cast<int>(), font, option)];
      if (!ptr)
        ptr.reset(new SimpleTextLayout(text, size, font, option));
      layout = ptr.get();
    }

    layout->generate();

    return *layout;
  }

  void SimpleTextLayout::generate()
  {
    if (!isLayoutReady()) {
      m_d->layout(maximumSize());
      setBoundingBox(m_d->m_layout.boundingRect());
      auto align = m_d->m_layout.textOption().alignment();
      /// @todo how about clipping?
      if (align & Qt::AlignBottom) {
        setRenderLocation(Nimble::Vector2f(0, maximumSize().y - boundingBox().height()));
      } else if (align & Qt::AlignVCenter) {
        setRenderLocation(Nimble::Vector2f(0, 0.5f * (maximumSize().y - boundingBox().height())));
      } else {
        setRenderLocation(Nimble::Vector2f(0, 0));
      }
      setLayoutReady(true);
      clearGlyphs();
    }

    if (isComplete())
      return;

    clearGlyphs();

    const Nimble::Vector2f layoutLocation(m_d->m_layout.position().x(), m_d->m_layout.position().y());
    bool missingGlyphs = false;

    foreach (const QGlyphRun & glyphRun, m_d->m_layout.glyphRuns())
      missingGlyphs |= generateGlyphs(layoutLocation, glyphRun);

    setGlyphsReady(!missingGlyphs);
  }
} // namespace Luminous
