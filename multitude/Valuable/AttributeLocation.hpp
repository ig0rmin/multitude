/* COPYRIGHT
 *
 * This file is part of Valuable.
 *
 * Copyright: MultiTouch Oy, Helsinki University of Technology and others, 2007-2013
 *
 * This file is licensed under GNU Lesser General Public
 * License (LGPL), version 2.1. The LGPL conditions can be found in 
 * file "LGPL.txt" that is distributed with this source package or obtained 
 * from the GNU organization (www.gnu.org).
 * 
 */

#ifndef VALUABLE_ATTRIBUTE_LOCATION_HPP
#define VALUABLE_ATTRIBUTE_LOCATION_HPP

#include "AttributeVector.hpp"

namespace Valuable
{

  /// This class provides an attribute for Nimble::Vector2f.
  class AttributeLocation2f : public AttributeVector<Nimble::Vector2f>
  {
  public:
    using AttributeVector<Nimble::Vector2f>::operator =;

    AttributeLocation2f(Node * host, const QByteArray & name,
                        const Nimble::Vector2f & v = Nimble::Vector2f(0, 0),
                        bool transit = false)
      : AttributeVector<Nimble::Vector2f>(host, name, v, transit)
      , m_src(0, 0)
    {
      for(int i = 0; i < Attribute::LAYER_COUNT; ++i)
        for(int j = 0; j < m_factors[i].Elements; ++j)
          m_factors[i][j] = std::numeric_limits<float>::quiet_NaN();
    }

    virtual bool set(float v, Layer layer = USER, ValueUnit unit = VU_UNKNOWN) OVERRIDE
    {
      const Nimble::Vector2f f(v, v);
      if(unit == VU_PERCENTAGE) {
        m_factors[layer] = f;
        setValue(Nimble::Vector2(f.x * m_src.x, f.y * m_src.y), layer);
      } else {
        for(int j = 0; j < m_factors[layer].Elements; ++j)
          m_factors[layer][j] = std::numeric_limits<float>::quiet_NaN();
        setValue(f, layer);
      }
      return true;
    }

    virtual bool set(int v, Layer layer = USER, ValueUnit = VU_UNKNOWN) OVERRIDE
    {
      for(int j = 0; j < m_factors[layer].Elements; ++j)
        m_factors[layer][j] = std::numeric_limits<float>::quiet_NaN();
      setValue(Nimble::Vector2f(v, v), layer);
      return true;
    }

    virtual bool set(const Nimble::Vector2f & v, Layer layer = USER, QList<ValueUnit> units = QList<ValueUnit>()) OVERRIDE
    {
      Nimble::Vector2f f(v);
      for(int j = 0; j < m_factors[layer].Elements; ++j) {
        if(j < units.size() && units[j] == VU_PERCENTAGE) {
          m_factors[layer][j] = f[j];
          f[j] *= m_src[j];
        } else {
          m_factors[layer][j] = std::numeric_limits<float>::quiet_NaN();
        }
      }
      setValue(f, layer);
      return true;
    }

    void setSrcx(float src)
    {
      m_src.x = src;
      for(Attribute::Layer l = Attribute::DEFAULT; l < Attribute::LAYER_COUNT;
          l = Attribute::Layer(l + 1)) {
        if(!m_valueSet[l]) continue;
        if(!Nimble::Math::isNAN(m_factors[l].x))
          this->setValue(Nimble::Vector2f(src * m_factors[l].x, value().y), l);
      }
    }

    void setSrcy(float src)
    {
      m_src.y = src;
      for(Attribute::Layer l = Attribute::DEFAULT; l < Attribute::LAYER_COUNT;
          l = Attribute::Layer(l + 1)) {
        if(!m_valueSet[l]) continue;
        if(!Nimble::Math::isNAN(m_factors[l].y))
          this->setValue(Nimble::Vector2f(value().x, src * m_factors[l].y), l);
      }
    }

    void setSrc(Nimble::Vector2f src)
    {
      setSrcx(src.x);
      setSrcy(src.y);
    }

    void setX(float x, Layer layer = USER, ValueUnit unit = VU_PXS)
    {
      Nimble::Vector2f f(x, value().y);
      if (unit == VU_PERCENTAGE) {
        m_factors[layer][0] = x;
        f[0] *= m_src[0];
      } else {
        m_factors[layer][0] = std::numeric_limits<float>::quiet_NaN();
      }
      setValue(f, layer);
    }

    void setY(float y, Layer layer = USER, ValueUnit unit = VU_PXS)
    {
      Nimble::Vector2f f(value().x, y);
      if (unit == VU_PERCENTAGE) {
        m_factors[layer][1] = y;
        f[1] *= m_src[1];
      } else {
        m_factors[layer][1] = std::numeric_limits<float>::quiet_NaN();
      }
      setValue(f, layer);
    }

    virtual void clearValue(Attribute::Layer layer = USER) OVERRIDE
    {
      for(int j = 0; j < m_factors[layer].Elements; ++j)
        m_factors[layer][j] = std::numeric_limits<float>::quiet_NaN();
      AttributeVector<Nimble::Vector2f>::clearValue(layer);
    }

  private:
    Nimble::Vector2f m_factors[Attribute::LAYER_COUNT];
    Nimble::Vector2f m_src;
  };

}

#endif // VALUABLE_ATTRIBUTE_LOCATION_HPP
