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

#ifndef VALUABLE_VALUE_VECTOR_HPP
#define VALUABLE_VALUE_VECTOR_HPP

#include <Valuable/Export.hpp>
#include <Valuable/ValueObject.hpp>

#include <Nimble/Vector4.hpp>

#include <Radiant/StringUtils.hpp>

#include <sstream>

#define VALUEMIT_STD_OP emitChange(); return *this;

namespace Valuable
{

  /** A template class for vevctor values.

      This class is used to implement all the normal vector value
      objects.
   */
  template<class VectorType, typename ElementType, int N>
  class VALUABLE_API ValueVector : public ValueObject
  {
    public:
      ValueVector() : ValueObject() {}
      /// @copydoc ValueObject::ValueObject(HasValues *, const std::string &, bool transit)
      ValueVector(HasValues * parent, const std::string & name, const VectorType & v = VectorType(), bool transit = false) : ValueObject(parent, name, transit), m_value(v) {}

      virtual ~ValueVector();

      /// Assigns a vector
      ValueVector<VectorType, ElementType, N> & operator =
      (const VectorType & v) { m_value = v; emitChange(); return *this; }

      /// Assigns by addition
      ValueVector<VectorType, ElementType, N> & operator +=
      (const VectorType & v) { m_value += v; emitChange(); return *this; }
      /// Assigns by subtraction
      ValueVector<VectorType, ElementType, N> & operator -=
      (const VectorType & v) { m_value -= v; emitChange(); return *this; }

    /// Subtract
      VectorType operator -
      (const VectorType & v) const { return m_value - v; }
      /// Add
    VectorType operator +
      (const VectorType & v) const { return m_value + v; }

    /// Returns the ith element
    ElementType operator [] (int i) const { return m_value[i]; }

    /// Returns the data in its native format
     const ElementType * native() const
    { return m_value.data(); }

    virtual void processMessage(const char * id, Radiant::BinaryData & data);
    virtual bool deserializeXML(DOMElement element);

    const char * type() const;

    /// Sets the value
      virtual bool set(const VectorType & v);

      /** Returns the internal vector object as a constant reference. */
      const VectorType & asVector() const { return m_value; }
      /** Returns the internal vector object as a constant reference. */
      const VectorType & operator * () const { return m_value; }

      std::string asString(bool * const ok = 0) const;

      /// Returns the ith element
      inline const ElementType & get(int i) const { return m_value[i]; }
      /// Returns a pointer to the first element
      inline const ElementType * data() const { return m_value.data(); }

      /// Returns the first component
      inline const ElementType & x() const { return m_value[0]; }
      /// Returns the second component
      inline const ElementType & y() const { return m_value[1]; }

      /// Normalizes the vector
      inline void normalize(ElementType len = 1.0)
      {
        m_value.normalize(len);
        emitChange();
      }

      /// @todo use ValueTyped<T> or something similar instead
      operator const VectorType & () const { return m_value; }
      const VectorType * operator-> () const { return &m_value; }
    private:
      VectorType m_value;
  };

  /// An integer vector2 value object
  typedef ValueVector<Nimble::Vector2i, int, 2> ValueVector2i;
  /// An integer vector3 value object
  typedef ValueVector<Nimble::Vector3i, int, 3> ValueVector3i;
  /// An integer vector4 value object
  typedef ValueVector<Nimble::Vector4i, int, 4> ValueVector4i;

  /// A float vector2 value object
  typedef ValueVector<Nimble::Vector2f, float, 2> ValueVector2f;
  /// A float vector3 value object
  typedef ValueVector<Nimble::Vector3f, float, 3> ValueVector3f;
  /// A float vector4 value object
  typedef ValueVector<Nimble::Vector4f, float, 4> ValueVector4f;

}

#undef VALUEMIT_STD_OP

#endif
