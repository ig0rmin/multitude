/* Copyright (C) 2007-2013: Multi Touch Oy, Helsinki University of Technology
 * and others.
 *
 * This file is licensed under GNU Lesser General Public License (LGPL),
 * version 2.1. The LGPL conditions can be found in file "LGPL.txt" that is
 * distributed with this source package or obtained from the GNU organization
 * (www.gnu.org).
 *
 */

#ifndef VALUABLE_ATTRIBUTESTRINGMAP_HPP
#define VALUABLE_ATTRIBUTESTRINGMAP_HPP

#include "Export.hpp"
#include "Attribute.hpp"

#include <QMap>

namespace Valuable
{
  /// This class provides a QMap<QString, QString> attribute.
  /// In CSS map entries are pairs of strings separated by colons. For example in
  /// CSS one would define the values in following way:
  /// @code
  /// attribute : "key" "value", "key with spaces" "value with spaces";
  /// @endcode
  template <>
  class VALUABLE_API AttributeT<QMap<QString, QString> > : public AttributeBaseT<QMap<QString, QString> >
  {
    typedef QMap<QString, QString> StringMap;
    typedef AttributeBaseT<StringMap> Base;

  public:
    using Base::operator =;

    AttributeT();
    AttributeT(Node * host, const QByteArray & name,
               const StringMap & v = StringMap(), bool transit = false);

    /// Returns the value as string
    virtual QString asString(bool * const ok, Layer layer) const OVERRIDE;

    virtual bool set(const QString & v, Layer layer = USER, ValueUnit unit = VU_UNKNOWN) OVERRIDE;
    virtual bool set(const StyleValue & v, Layer layer = USER) OVERRIDE;
  };
  typedef AttributeT<QMap<QString, QString> > AttributeStringMap;
} // namespace Valuable

#endif // VALUABLE_ATTRIBUTESTRINGLIST_HPP
