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

#ifndef VALUABLE_VALUE_OBJECT_HPP
#define VALUABLE_VALUE_OBJECT_HPP

#include "Export.hpp"
#include "Archive.hpp"
#include "ValueListener.hpp"
#include "DOMElement.hpp"

#include <Nimble/Vector4.hpp>

#include <Patterns/NotCopyable.hpp>

#include <Radiant/BinaryData.hpp>
#include <Radiant/MemCheck.hpp>

#include <set>
#include <string>

#ifdef MULTI_DOCUMENTER
#include "Serializer.hpp"
#include "XMLArchive.hpp"

// some forward declarations to work around cyclic include problems
namespace Valuable
{
  class Serializable;
  namespace Serializer
  {
    template <typename T>
    ArchiveElement serialize(Archive & archive, const T & t);
  }
}

#endif

namespace Valuable
{
  class HasValues;
  //class DOMElement;
  class DOMDocument;


  /// The base class for all serializable objects.
  class VALUABLE_API Serializable
  {
    MEMCHECKED
  public:
    virtual ~Serializable() {}

    /// Serializes (writes) this object to an element.
    /// @param archive The serializer archive that is used to create the new
    ///                element and maintains the serialization state and options.
    /// @return The new serialized element.
    virtual ArchiveElement serialize(Archive & archive) const = 0;

    /// Deserializes (reads) this object from serializer element.
    /// @param element Serialized element that holds the data that should be deserialized.
    /// @return Returns true if the read process worked correctly, and false otherwise.
    virtual bool deserialize(const ArchiveElement & element) = 0;

    /// Deserializes (reads) this object from an XML element.
    /// This function is only for keeping backwards compatibility.
    /// @param element XML element that is deserialized
    /** @return Returns true if the read process worked correctly, and false otherwise. */
    virtual bool deserializeXML(const DOMElement & element);
  };


  /** The base class for value objects.

      Typical child classes include some POD (plain old data) elements
      (floats, ints, vector2) etc, that can be accessed through the
      API. The ValueObjects have names (std::string), that can be used to access
      ValueObjects that are stored inside HasValues objects.

      It is also possible to add listeners to values, so that if a
      value is changed, then a call-back to soem other object is
      followed. The listener-API is a bit hard-core, but it has plenty
      of power when you need to track the state of other objects.

      @see HasValues
  */

  /// @todo the "set" functions are duplicating the processMessage functionality
  /// @todo processMessage should be renamed to eventProcess (can be tricky to do)
  /// @todo Doc
  class VALUABLE_API ValueObject : public Serializable
  {
  public:
    ValueObject();
    /// The copy constructor creates a copy of the ValueObject WITHOUT the
    /// link to host
    ValueObject(const ValueObject & o);
    /** Constructs a new value object and attaches it to its host.

    @param host The host object. This object is automatically
    added to the host.

    @param name The name (or id) of this value. Names are typically
    human-readable. The names should not contain white-spaces
    as they may be used in XML files etc.

    @param transit Should value changes be transmitted forward. This
    is related to future uses, and can be largely ignored at the
    moment.
    */
    ValueObject(HasValues * host, const std::string & name, bool transit = false);
    virtual ~ValueObject();

    /// Returns the name of the object.
    const std::string & name() const { return m_name; }
    /// Sets the name of the object
    void setName(const std::string & s);
    /// Returns the path (separated by '/'s) from the root
    std::string path() const;

    /// Process a message
    /** This method is a key element in the event-passing system.
        It is used to deliver information between objects. The information contains
        two parts:

        <OL>
        <LI>Identifier: A character string that gives the address for the adjustment</LI>
        <LI>Data: A binary blob that contains information for the message</LI>
        </OL>

        This function is overridden in number of classes that need to receive and
        process events. In a typical case, when overriding this function, you should
        either process the message, or call the function of the parent class.

        \code
        void MyClass::processMessage(const char * type, Radiant::BinaryData & data)
        {
          if(strcmp(type, "jump") == 0)
            doJump();
          else if(strcmp(type, "crawl") == 0) {
            bool ok;
            int speed = data.readInt32(&ok);
            if(ok)
              doCrawl(speed);
          }
          else
            Parent::processMessage(type, data);
        }
        \endcode

        @param id The indentifier for the message. Typically this is quite human-readable

        @param data Binary blob that contains the argument data in easily parseable format.

    */
    virtual void processMessage(const char * id, Radiant::BinaryData & data);
    /// Utility function for sending string message to the object
    void processMessageString(const char * id, const char * str);
    /// Utility function for sending a float message to the object
    void processMessageFloat(const char * id, float v);
    /// Utility function for sending an int message to the object
    void processMessageInt(const char * id, int v);
    /// Utility function for sending a Vector2 message to the object
    void processMessageVector2(const char * id, Nimble::Vector2);
    /// Utility function for sending a Vector3 message to the object
    void processMessageVector3(const char * id, Nimble::Vector3);
    /// Utility function for sending a Vector4 message to the object
    void processMessageVector4(const char * id, Nimble::Vector4);

    /// Converts the value object in a floating point number
    /// @param ok If non-null, *ok is set to true/false on success/error
    /// @return Object as a float, the default implementation returns 0.0f
    virtual float       asFloat(bool * const ok = 0) const;
    /// Converts the value object in an integer
    /// @param ok If non-null, *ok is set to true/false on success/error
    /// @return Object as a int, the default implementation returns zero
    virtual int         asInt(bool * const ok = 0) const;
    /// Converts the value object to a string
    /// @param ok If non-null, *ok is set to true/false on success/error
    /// @return Object as a string, the default implementation returns an empty string
    virtual std::string asString(bool * const ok = 0) const;

    /// Sets the value of the object
    virtual bool set(float v);
    /// Sets the value of the object
    virtual bool set(int v);
    /// Sets the value of the object
    virtual bool set(const std::string & v);
    /// Sets the value of the object
    virtual bool set(const Nimble::Vector2f & v);
    /// Sets the value of the object
    virtual bool set(const Nimble::Vector4f & v);

    /// Get the type id of the type
    virtual const char * type() const = 0;

    /// The object is serialized using its name as a tag name.
    /// @param archive Serialization archive that is used to create new elements.
    /// @return Serialized object as an ArchiveElement
    virtual ArchiveElement serialize(Archive & archive) const;

    /// The host object of the value object (is any).
    /// @return Pointer to the host
    HasValues * host() const { return m_host; }
    /** Sets the host pointer to zero and removes this object from the host. */
    void removeHost();

    /// Adds a listener that is invoked whenever the value is changed
    void addListener(ValueListener * l) { m_listeners.push_back(l); }
    /// Removes a listener from the listener list
    void removeListener(ValueListener * l) { m_listeners.remove(l); }

    /// Returns true if the current value of the object is different from the original value.
    virtual bool isChanged() const;

#ifdef MULTI_DOCUMENTER
    struct Doc
    {
      std::string class_name;
      std::string orig_str;
      HasValues * obj;
      ValueObject * vo;
    };

    static std::list<Doc> doc;
#endif

  protected:

    /// Invokes the change valueChanged function of all listeners
    virtual void emitChange();
    /// Invokes the change valueDeleted function of all listeners
    virtual void emitDelete();

  private:
    // The object that holds this object
    HasValues * m_host;
    bool m_changed;
    std::string m_name;
    bool m_transit;

    ValueListeners m_listeners;

    friend class HasValues;
  };

  /// Every ValueObject is some kind of ValueObjectT<T> object.
  /// Common functionality should be in either here or in ValueObject
  template <typename T> class ValueObjectT : public ValueObject
  {
  public:
    /// Creates a new ValueObjectT and stores the original and current value as a separate variables.
    /// @param host host object
    /// @param name name of the value
    /// @param v the default/original value of the object
    /// @param transit ignored
    ValueObjectT(HasValues * host, const std::string & name, const T & v = T(), bool transit = false)
      : ValueObject(host, name, transit),
      m_value(v),
      m_orig(v) {
#ifdef MULTI_DOCUMENTER
      Doc & d = doc.back();
      XMLArchive archive;
      ArchiveElement e = Serializer::serialize<T>(archive, m_orig);
      if(!e.isNull()) {
        d.orig_str = e.get();
      }
#endif
    }

    ValueObjectT()
      : ValueObject() {}

    virtual ~ValueObjectT() {}

    /// Access the wrapped object with the dereference operator
    inline const T & operator * () const { return m_value; }
    /// Typecast operator for the wrapped value
    inline operator T & () { return m_value; }
    /// Typecast operator for the wrapped value
    inline operator const T & () const { return m_value; }
    /// Use the arrow operator to access fields inside the wrapped object.
    inline const T * operator->() const { return &m_value; }

    /// The original value (the value given in constructor) of the ValueObject.
    inline const T & orig() const { return m_orig; }

    /// Is the value different from the original value
    // use !( == ) instead of != because != isn't always implemented
    // virtual bool isChanged() const { return !(m_value == m_orig); }

  protected:
    /// The actual value of this object
    T m_value;
    /// The original value given in constructor
    T m_orig;
  };


}

#endif
