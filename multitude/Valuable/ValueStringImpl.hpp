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

#ifndef VALUABLE_VALUE_STRING_IMPL_HPP
#define VALUABLE_VALUE_STRING_IMPL_HPP

#include "ValueString.hpp"
#include "DOMElement.hpp"
#include "DOMDocument.hpp"

#include <Radiant/StringUtils.hpp>

#define STD_EM this->emitChange();
#define VALUEMIT_STD_OP this->emitChange(); return *this;

namespace Valuable
{

  
  template<class T>
  ValueStringT<T>::ValueStringT(HasValues * parent, const std::string & name,
				const T & v, bool transit)
    : ValueObject(parent, name, transit), m_value(v)
  {}

  template<class T>
  ValueStringT<T>::ValueStringT(HasValues * parent, const std::string & name,
				const char * v, bool transit)
  : ValueObject(parent, name, transit),
    m_value(v)
  {}

    
  template<class T>
  ValueStringT<T>::ValueStringT(HasValues * parent, const std::string & name,
				bool transit)
    : ValueObject(parent, name, transit)
  {}


  template<class T>
  void ValueStringT<T>::processMessage(const char * id, Radiant::BinaryData & data)
  {
      (void) id;
      bool ok = true;
      T tmp = data.read<T>(&ok);
      if(ok)
	*this = tmp;
      /*
      Radiant::info("ValueStringT<T>::processMessage # Ok = %d %s",
                    (int) ok, m_value.c_str());
      */
  }

  template<class T>
  bool ValueStringT<T>::deserialize(ArchiveElement & element)
  {
    m_value = T(element.get());

    STD_EM;

    return true;
  }

  template<class T>
  float ValueStringT<T>::asFloat(bool * const ok) const 
  { 
    if(ok) *ok = true; 
    return float(atof(m_value.c_str()));
  }

  template<class T>
  int ValueStringT<T>::asInt(bool * const ok) const 
  {
    if(ok) *ok = true; 
    return atoi(m_value.c_str());
  }

  template<class T>
  std::string ValueStringT<T>::asString(bool * const ok) const 
  { 
    if(ok) *ok = true; 
    return m_value; 
  }

  template<class T>
  std::wstring ValueStringT<T>::asWString(bool * const ok) const 
  { 
    if(ok) *ok = true; 
    std::wstring tmp;
    Radiant::StringUtils::utf8ToStdWstring(tmp, m_value);
    return tmp; 
  }

  template<class T>
  bool ValueStringT<T>::set(const std::string & v) 
  { 
    m_value = T(v); 
    STD_EM;
    return true; 
  }

  /////////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////////

  // Specializations for wide strings.

  /*
  template <>
  ValueStringT<std::wstring>::ValueStringT()
  : ValueObject()
  {}
  
  template <>
  ValueStringT<std::wstring>::ValueStringT(HasValues * parent, const std::string & name, const std::wstring & v, bool transit)
  : ValueObject(parent, name, transit),
  m_value(v)
  {}
  */

  /// @copydoc ValueObject::ValueObject(HasValues *, const std::string &, bool transit)
  template<>
  ValueStringT<std::wstring>::ValueStringT(HasValues * parent, const std::string & name,
					   const char * v, bool transit)
    : ValueObject(parent, name, transit)
  {
    std::string tmp(v);
    Radiant::StringUtils::utf8ToStdWstring(m_value, tmp); 
  }

  /// Assigns a string
  template <>
  ValueStringT<std::wstring> & ValueStringT<std::wstring>::operator=(const ValueStringT<std::wstring> & i)
  {
    m_value = i.m_value;
    VALUEMIT_STD_OP
  }

  /// Assigns a string
  template <>
  ValueStringT<std::wstring> & ValueStringT<std::wstring>::operator=(const std::wstring & i)
  {
    m_value = i;
    VALUEMIT_STD_OP
  }

  /// Converts the string to float
  template <>
  float ValueStringT<std::wstring>::asFloat(bool * const ok) const 
  { 
    if(ok) *ok = true;
    std::string tmp;
    Radiant::StringUtils::stdWstringToUtf8(tmp, m_value);
    return float(atof(tmp.c_str()));
  }

  /// Converts the string to integer
  template <>
  int ValueStringT<std::wstring>::asInt(bool * const ok) const 
  { 
    if(ok) *ok = true; 
    std::string tmp;
    Radiant::StringUtils::stdWstringToUtf8(tmp, m_value);
    return atoi(tmp.c_str()); 
  }

  /// Converts the wide-byte string to ascii string
  template<>
  std::string ValueStringT<std::wstring>::asString(bool * const ok) const 
  { 
    if(ok) *ok = true; 
    std::string tmp;
    Radiant::StringUtils::stdWstringToUtf8(tmp, m_value);
    return tmp; 
  }

  /// Converts the wide-byte string to ascii string
  template<>
  std::wstring ValueStringT<std::wstring>::asWString(bool * const ok) const 
  { 
    if(ok) *ok = true; 
    return m_value;
  }

  template<>
  bool ValueStringT<std::wstring>::set(const std::string & v) 
  { 
    Radiant::StringUtils::utf8ToStdWstring(m_value, v); 
    STD_EM;
    return true; 
  }

  template<>
  ArchiveElement & ValueStringT<std::string>::serialize(Archive & archive)
  {
    return ValueObject::serialize(archive);
  }

  template<>
  ArchiveElement & ValueStringT<std::wstring>::serialize(Archive & archive)
  {
    if(name().empty()) {
      Radiant::error("ValueWString::serialize # attempt to serialize object with no name");
      return archive.emptyElement();
    }

    ArchiveElement & elem = archive.createElement(name().c_str());
    elem.add("type", type());
  
    std::wstring ws = asWString();
    elem.set(ws);

    return elem;
  }

  template<>
  bool ValueStringT<std::wstring>::deserialize(ArchiveElement & element)
  {
    m_value = element.getW();
    STD_EM;
    return true;
  }
 
}

#undef VALUEMIT_STD_OP
#undef STD_EN

#endif
