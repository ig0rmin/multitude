/* COPYRIGHT
 */

#ifndef VALUABLE_VALUE_RECT_HPP
#define VALUABLE_VALUE_RECT_HPP

#include <Valuable/Export.hpp>
#include <Valuable/ValueObject.hpp>

#include <Nimble/Rect.hpp>

namespace Valuable
{

  /// A valuable object holding a Nimble::Rect object
  template <class T>
  class VALUABLE_API ValueRectT : public ValueObjectT<Nimble::RectT<T> >
  {
    typedef ValueObjectT<Nimble::RectT<T> > Base;
  public:
    using Base::operator =;

    /// @copydoc ValueObject::ValueObject(HasValues *, const std::string &, bool transit)
    /// @param r The rectangle to be stored in the ValueRect
    ValueRectT(HasValues * host, const QString & name, const Nimble::RectT<T> & r, bool transit = false);

    const char * type() const;

    QString asString(bool * const ok = 0) const;

    bool deserialize(const ArchiveElement & element);

    /// Converts the object to rectangle
    Nimble::RectT<T> asRect() const { return this->value(); }
  };

  /// Default floating point ValueRectT typedef
  typedef ValueRectT<float> ValueRect;
  /// ValueRectT of floats
  typedef ValueRectT<float> ValueRectf;
  /// ValueRectT of doubles
  typedef ValueRectT<double> ValueRectd;
  /// ValueRectT of ints
  typedef ValueRectT<int> ValueRecti;
}

#endif
