#ifndef VALUABLE_VALUE_CONTAINER_HPP
#define VALUABLE_VALUE_CONTAINER_HPP

#include "Serializer.hpp"
#include "ValueObject.hpp"

namespace Valuable
{
  template<typename T> class ValueContainer : public ValueObject
  {
  public:
    typedef typename T::iterator iterator;
    typedef typename T::const_iterator const_iterator;
    typedef typename T::reverse_iterator reverse_iterator;
    typedef typename T::value_type value_type;

    ValueContainer() {}

    ValueContainer(HasValues * parent, const std::string & name)
      : ValueObject(parent, name, false)
    {}

    virtual const char* type() const { return "container"; }

    virtual DOMElement serializeXML(DOMDocument * doc)
    {
      DOMElement elem = doc->createElement((name().empty() ? type() : name()).c_str());
      for(iterator it = m_container.begin(); it != m_container.end(); it++) {
        elem.appendChild(Serializer::serializeXML(doc, *it));
      }
      return elem;
    }

    virtual bool deserializeXML(DOMElement element)
    {
      DOMElement::NodeList list = element.getChildNodes();
      std::insert_iterator<T> inserter(m_container, m_container.end());
      for(DOMElement::NodeList::iterator it = list.begin(); it != list.end(); it++) {
        *inserter = Serializer::deserializeXML<typename T::value_type>(*it);
      }
      return true;
    }

    /// @todo should be in ValueTyped
    operator T & () { return m_container; }
    operator const T & () const { return m_container; }

    T & operator * () { return m_container; }
    const T & operator * () const { return m_container; }

    T * operator -> () { return &m_container; }
    const T * operator -> () const { return &m_container; }

  protected:
    T m_container;
  };
}

#endif // VALUABLE_VALUE_CONTAINER_HPP
