#ifndef VALUABLE_ATTRIBUTESTRINGLIST_HPP
#define VALUABLE_ATTRIBUTESTRINGLIST_HPP

#include "Export.hpp"
#include "AttributeObject.hpp"

#include <QStringList>

namespace Valuable
{
  /// This class provides a QStringList attribute.
  class VALUABLE_API AttributeStringList : public AttributeT<QStringList>
  {
    typedef AttributeT<QStringList> Base;

  public:
    using Base::operator =;

    AttributeStringList();
    AttributeStringList(Node * host, const QByteArray & name,
                        const QStringList & v = QStringList(), bool transit = false);

    /// Returns the value as string
    virtual QString asString(bool * const ok = 0) const OVERRIDE;

    virtual bool set(const QString & v, Layer layer = USER, ValueUnit unit = VU_UNKNOWN) OVERRIDE;
    virtual bool set(const StyleValue & v, Layer layer = USER) OVERRIDE;
  };
} // namespace Valuable

#endif // VALUABLE_ATTRIBUTESTRINGLIST_HPP
