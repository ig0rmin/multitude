/* COPYRIGHT
 *
 * This file is part of Valuable.
 *
 * Copyright: MultiTouch Oy, Helsinki University of Technology and others.
 *
 * See file "Valuable.hpp" for authors and more details.
 *
 * This file is licensed under GNU Lesser General Public
 * License (LGPL), version 2.1. The LGPL conditions can be found in
 * file "LGPL.txt" that is distributed with this source package or obtained
 * from the GNU organization (www.gnu.org).
 *
 */

#ifndef VALUABLE_ATTRIBUTE_FRAME_HPP
#define VALUABLE_ATTRIBUTE_FRAME_HPP

#include <Nimble/Frame4.hpp>

#include "AttributeFloat.hpp"
#include "AttributeVector.hpp"
#include "StyleValue.hpp"

#include <QStringList>

#include <array>

namespace Valuable
{
  /// This class provides an attribute that stores a two-dimensional frame. The
  /// frame width can be individually defined for top, right, bottom, and left
  /// edges of the frame. This class is used by the CSS engine.
  class AttributeFrame : public Attribute
  {
  public:
    AttributeFrame(Node * host, const QByteArray & name,
                   const Nimble::Frame4f & v = Nimble::Frame4f(), bool transit = false)
      : Attribute(host, name, transit)
      , m_inChangeTransaction(false)
      , m_emitChangedAfterTransaction(false)
    {
      m_values[0] = new AttributeFloat(host, name + "-top", v.top(), transit);
      m_values[1] = new AttributeFloat(host, name + "-right", v.right(), transit);
      m_values[2] = new AttributeFloat(host, name + "-bottom", v.bottom(), transit);
      m_values[3] = new AttributeFloat(host, name + "-left", v.left(), transit);

#ifndef CLANG_XML
      for (int i = 0; i < 4; ++i) {
        m_values[i]->addListener(std::bind(&AttributeFrame::valuesChanged, this));
        m_values[i]->setSerializable(false);
      }
#endif
    }

    ~AttributeFrame()
    {
      for(int i = 0; i < 4; ++i)
        delete m_values[i];
    }

    virtual QString asString(bool * const ok = 0) const OVERRIDE
    {
      if (ok) *ok = true;
      return QString("%1 %2 %3 %4").arg(*m_values[0]).arg(*m_values[1]).arg(*m_values[2]).arg(*m_values[3]);
    }

    virtual bool deserialize(const ArchiveElement & element) OVERRIDE
    {
      QStringList lst = element.get().split(QRegExp("\\s"), QString::SkipEmptyParts);
      if (lst.size() == 4) {
        Nimble::Frame4f frame;
        for (int i = 0; i < 4; ++i) {
          bool ok = false;
          frame[i] = lst[i].toFloat(&ok);
          if (!ok)
            return false;
        }
        *this = frame;

        return true;
      }

      return false;
    }

    virtual bool set(float v, Layer layer = USER, ValueUnit unit = VU_UNKNOWN) OVERRIDE
    {
      beginChangeTransaction();

      for (int i = 0; i < 4; ++i)
        m_values[i]->set(v, layer, unit);

      endChangeTransaction();

      return true;
    }

    virtual bool set(int v, Layer layer = USER, ValueUnit unit = VU_UNKNOWN) OVERRIDE
    {
      beginChangeTransaction();

      for (int i = 0; i < 4; ++i)
        m_values[i]->set(v, layer, unit);

      endChangeTransaction();

      return true;
    }

    /// @todo all of the set() functions will crash if you call them with default arguments (units is empty)
    virtual bool set(const Nimble::Vector2f & v, Layer layer = USER, QList<ValueUnit> units = QList<ValueUnit>()) OVERRIDE
    {
      beginChangeTransaction();

      for (int i = 0; i < 4; ++i)
        m_values[i]->set(v[i % 2], layer, units[i % 2]);

      endChangeTransaction();

      return true;
    }

    virtual bool set(const Nimble::Vector3f & v, Layer layer = USER, QList<ValueUnit> units = QList<ValueUnit>()) OVERRIDE
    {
      beginChangeTransaction();

      for (int i = 0; i < 4; ++i)
        m_values[i]->set(v[i % 3], layer, units[i % 3]);

      endChangeTransaction();

      return true;
    }

    virtual bool set(const Nimble::Vector4f & v, Layer layer = USER, QList<ValueUnit> units = QList<ValueUnit>()) OVERRIDE
    {
      beginChangeTransaction();

      for (int i = 0; i < 4; ++i)
        m_values[i]->set(v[i], layer, units[i]);

      endChangeTransaction();

      return true;
    }

    virtual bool isChanged() const OVERRIDE
    {
      for (int i = 0; i < 4; ++i)
        if (m_values[i]->isChanged())
          return true;
      return false;
    }

    virtual void clearValue(Layer layout) OVERRIDE
    {
      beginChangeTransaction();

      for (int i = 0; i < 4; ++i)
        m_values[i]->clearValue(layout);

      endChangeTransaction();
    }

    virtual void setAsDefaults() OVERRIDE
    {
      beginChangeTransaction();

      for (int i = 0; i < 4; ++i)
        m_values[i]->setAsDefaults();

      endChangeTransaction();
    }

    void setSrc(float src)
    {
      beginChangeTransaction();

      for (int i = 0; i < 4; ++i)
        m_values[i]->setSrc(src);

      endChangeTransaction();
    }

    void setSrc(const Nimble::Vector4f & src)
    {
      beginChangeTransaction();

      for (int i = 0; i < 4; ++i)
        m_values[i]->setSrc(src[i]);

      endChangeTransaction();
    }

    AttributeFrame & operator=(const AttributeFrame & frame)
    {
      beginChangeTransaction();

      for (int i = 0; i < 4; ++i)
        *m_values[i] = **frame.m_values[i];

      endChangeTransaction();

      return *this;
    }

    AttributeFrame & operator=(const Nimble::Frame4f & frame)
    {
      beginChangeTransaction();

      for (int i = 0; i < 4; ++i)
        *m_values[i] = frame[i];

      endChangeTransaction();

      return *this;
    }

    Nimble::Frame4f operator*() const
    {
      return value();
    }

    Nimble::Frame4f value() const
    {
      return Nimble::Frame4f(*m_values[0], *m_values[1], *m_values[2], *m_values[3]);
    }

    bool handleShorthand(const StyleValue & value,
                         QMap<Attribute *, StyleValue> & expanded) OVERRIDE
    {
      if (value.size() > 0 && value.size() < 5) {
        for (int i = 0; i < 4; ++i)
          expanded[m_values[i]] = value[i % value.size()];
        return true;
      }
      return false;
    }

  private:
    void valuesChanged()
    {
      if(m_inChangeTransaction)
        m_emitChangedAfterTransaction = true;
      else
        Attribute::emitChange();
    }

    void beginChangeTransaction()
    {
      assert(m_inChangeTransaction == false);
      m_inChangeTransaction = true;
    }

    void endChangeTransaction()
    {
      assert(m_inChangeTransaction);

      m_inChangeTransaction = false;

      if(m_emitChangedAfterTransaction) {
        m_emitChangedAfterTransaction = false;
        Attribute::emitChange();
      }
    }

    std::array<AttributeFloat *, 4> m_values;

    bool m_inChangeTransaction;
    bool m_emitChangedAfterTransaction;
  };
}

#endif // VALUABLE_ATTRIBUTE_FRAME_HPP
