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

#ifndef VALUEMATRIX_HPP
#define VALUEMATRIX_HPP

#include <Valuable/Export.hpp>
#include <Valuable/ValueObject.hpp>

#include <Nimble/Matrix4.hpp>

namespace Valuable
{

  /// A matrix value object
  template<class MatrixType, typename ElementType, int N>
  class VALUABLE_API ValueMatrix : public ValueObjectT<MatrixType>
  {
    typedef ValueObjectT<MatrixType> Base;
  public:
    /// Create a new ValueMatrix
    /// @param host host object
    /// @param name name of the value
    /// @param v the default/original value of the object
    /// @param transit ignored
    ValueMatrix(HasValues * host, const std::string & name, const MatrixType & v = MatrixType(), bool transit = false)
      : Base(host, name, v, transit) {}

    ValueMatrix() : Base() {}
    virtual ~ValueMatrix();

    /// Returns the data in its native format
    const ElementType * native() const
    { return Base::m_value.data(); }

    // virtual void processMessage(const char * id, Radiant::BinaryData & data);
    virtual bool deserialize(const ArchiveElement & element);
    const char * type() const;
    /// Sets the ValueMatrix value
    /// @param v The new matrix that is copied to the ValueMatrix
    /// @return True
    virtual bool set(const MatrixType & v);
    std::string asString(bool * const ok = 0) const;

    const MatrixType & operator * () const { return Base::m_value; }

    /// Sets the ValueMatrix value
    /// @param v The new matrix that is copied to the ValueMatrix
    /// @return Reference to this
    ValueMatrix<MatrixType, ElementType, N> & operator =
        (const MatrixType & v) { Base::m_value = v; this->emitChange(); return *this; }

  };

  /// A float Matrix2 value object
  typedef ValueMatrix<Nimble::Matrix2f, float, 4> ValueMatrix2f;
  /// A float Matrix3 value object
  typedef ValueMatrix<Nimble::Matrix3f, float, 9> ValueMatrix3f;
  /// A float Matrix4 value object
  typedef ValueMatrix<Nimble::Matrix4f, float, 16> ValueMatrix4f;
}

#endif // VALUEMATRIX_HPP
