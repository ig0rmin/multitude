/* Copyright (C) 2007-2013: Multi Touch Oy, Helsinki University of Technology
 * and others.
 *
 * This file is licensed under GNU Lesser General Public License (LGPL),
 * version 2.1. The LGPL conditions can be found in file "LGPL.txt" that is
 * distributed with this source package or obtained from the GNU organization
 * (www.gnu.org).
 * 
 */

#ifndef VALUABLE_VALUE_ENUM_HPP
#define VALUABLE_VALUE_ENUM_HPP

#include "AttributeNumeric.hpp"
#include "AttributeFlags.hpp"

namespace Valuable
{
  class AttributeEnum
  {
  public:
    AttributeEnum() : m_allowIntegers(false) {}
    virtual ~AttributeEnum() {}

    void setAllowIntegers(bool allow)
    {
      m_allowIntegers = allow;
    }

    const QMap<QString, int> & enumValues() const { return m_enumValues; }

  protected:
    QMap<QString, int> m_enumValues;
    bool m_allowIntegers;
  };

  /**
   * Valuable enum. Similar to AttributeFlags, but only one value can be enabled
   * at a time.
   *
   * This class also supports pure integer values in addition to enum values.
   * In practice this means that you could write in CSS: "priority: low" or
   * "priority: 15".
   *
   * @code
   * /// FunnyWidget.hpp
   *
   * struct FunnyWidget {
   *   FunnyWidget();
   *
   *   enum Mode {
   *     ON = 1,
   *     OFF = 0
   *   };
   *
   *   enum {
   *     PriorityLow = 10,
   *     PriorityMedium = 50,
   *     PriorityHigh = 90
   *   };
   *
   *   /// m_mode is either ON or OFF
   *   AttributeT<Mode> m_mode;
   *
   *   /// m_priority is an integer, but has shortcuts low/medium/high
   *   AttributeEnumT<int> m_priority;
   * };
   *
   * //////////////////////////////////////////////////////////////////////////
   *
   * /// FunnyWidget.cpp
   *
   * /// In CSS/Script you can use keywords "on" or "enabled / "off" or "disabled"
   * Valuable::EnumNames s_modes[] = {{"on", FunnyWidget::ON, "enabled", FunnyWidget::ON},
   *                                  {"off", FunnyWidget::OFF, "disabled", FunnyWidget::OFF},
   *                                  {0, 0}};
   *
   * Valuable::EnumNames s_priorities[] = {{"low", FunnyWidget::PriorityLow},
   *                                       {"medium", FunnyWidget::PriorityMedium},
   *                                       {"high", FunnyWidget::PriorityHigh},
   *                                       {0, 0}};
   *
   * FunnyWidget::FunnyWidget()
   *  : m_mode(this, "mode", s_modes, ON)
   *  , m_priority(this, "priority", s_priorities, PriorityMedium)
   * {
   *   m_priority.setAllowIntegers(true);
   * }
   *
   * @endcode
   */
  template <typename T>
  class AttributeT<T, Attribute::ATTR_ENUM> : public AttributeNumericT<T>, public AttributeEnum
  {
    typedef AttributeNumericT<T> Base;

  public:
    using Base::operator =;
    using Base::value;

    AttributeT(Node * host, const QByteArray & name, const EnumNames * names,
                  const T & v, bool transit = false)
      : Base(host, name, v, transit),
        AttributeEnum()
    {
      for (const EnumNames * it = names; it->name; ++it) {
        m_enumValues[QString(it->name).toLower()] = it->value;
      }
    }

    virtual bool set(int v, Attribute::Layer layer = Attribute::USER,
                            Attribute::ValueUnit = Attribute::VU_UNKNOWN) OVERRIDE
    {
      if (m_allowIntegers)
        this->setValue(T(v), layer);
      return m_allowIntegers;
    }

    virtual bool set(const StyleValue & v, Attribute::Layer layer = Attribute::USER) OVERRIDE
    {
      if (v.size() != 1)
        return false;

      if (v.unit(0) != Attribute::VU_UNKNOWN)
        return false;

      auto it = m_enumValues.find(v.asString().toLower());
      if (it == m_enumValues.end())
        return false;

      this->setValue(T(*it), layer);
      return true;
    }

    virtual void eventProcess(const QByteArray &, Radiant::BinaryData & data) OVERRIDE
    {
      QString str;
      if (data.readString(str)) {
        auto it = m_enumValues.find(str.toLower());
        if (it != m_enumValues.end())
          this->setValue(T(*it));
      }
    }

    virtual QString asString(bool * const ok, Valuable::Attribute::Layer layer) const OVERRIDE
    {
      if (ok)
        *ok = true;

      T v = value(layer);
      for (auto it = m_enumValues.begin(); it != m_enumValues.end(); ++it) {
        if (*it == v)
          return it.key();
      }
      return QString::number(v);
    }
  };

}

#endif // VALUEENUM_HPP
