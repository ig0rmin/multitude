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

#ifndef VALUABLE_VALUE_STRING_HPP
#define VALUABLE_VALUE_STRING_HPP

#include <Valuable/Export.hpp>
#include <Valuable/ValueObject.hpp>

#include <QString>

#define VO_TYPE_STRING "string"

namespace Valuable
{

  /// String value
  class VALUABLE_API ValueString : public ValueObjectT<QString>
  {
    typedef ValueObjectT<QString> Base;

  public:

    /// The character type of this string class
    typedef QChar char_type;

    ValueString();
    /// @copydoc ValueObject::ValueObject(HasValues *, const QString &, bool transit)
    /// @param v The string to store in this object
    ValueString(HasValues * parent, const QString & name,
                const QString & v, bool transit = false);
    /// @copydoc ValueObject::ValueObject(HasValues *, const QString &, bool transit)
    /// @param v The string to store in this object
    ValueString(HasValues * parent, const QString & name,
                 const char * v, bool transit = false);
    /// @copydoc ValueObject::ValueObject(HasValues *, const QString &, bool transit)
    ValueString(HasValues * parent, const QString & name,
                 bool transit = false);

    virtual void processMessage(const char * id, Radiant::BinaryData & data);

    /// Copies a string
    ValueString & operator = (const ValueString & i);
    /// Copies a string
    ValueString & operator = (const QString & i);

    /// Concatenates two strings
    /// @param i The string to be appended to this string
    /// @return A new string that contains both this string, and the argument string.
    QString operator + (const ValueString & i) const;

    /// Concatenates two strings
    QString operator + (const QString & i) const;

    /// Concatenates strings with UTF-8 encoded string
    QString operator + (const char * utf8) const;

    /// Compares if two strings are equal
    bool operator == (const QString & that) const;
    /// Compares if two strings are not equal
    bool operator != (const QString & that) const;

    /// Returns the value as float
    float asFloat(bool * const ok = 0) const;
    /// Returns the value as integer
    int asInt(bool * const ok = 0) const;
    /// Returns the value as string
    QString asString(bool * const ok = 0) const;

    virtual bool set(const QString & v);

    const char * type() const { return VO_TYPE_STRING; }

    bool deserialize(ArchiveElement & element);

    /// Makes the string empty
    void clear();

    /// Returns the length of the string
    unsigned size() const;
  };
}

QString operator + (const QString & a, const Valuable::ValueString & b);

#endif
