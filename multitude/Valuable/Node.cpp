/* Copyright (C) 2007-2013: Multi Touch Oy, Helsinki University of Technology
 * and others.
 *
 * This file is licensed under GNU Lesser General Public License (LGPL),
 * version 2.1. The LGPL conditions can be found in file "LGPL.txt" that is
 * distributed with this source package or obtained from the GNU organization
 * (www.gnu.org).
 * 
 */

#include <Valuable/DOMDocument.hpp>
#include <Valuable/DOMElement.hpp>
#include <Valuable/Valuable.hpp>
#include <Valuable/Node.hpp>
#include <Valuable/Serializer.hpp>

#include <Radiant/Mutex.hpp>
#include <Radiant/TimeStamp.hpp>
#include <Radiant/Trace.hpp>
#include <memory>

#include <algorithm>
#include <typeinfo>
#include <cstring>

namespace
{
  struct QueueItem
  {
    QueueItem(Valuable::Node * sender_, Valuable::Node::ListenerFuncBd func_,
              const Radiant::BinaryData & data_)
      : sender(sender_)
      , func2(func_)
      , target()
      , data(data_)
    {}

    QueueItem(Valuable::Node * sender_, Valuable::Node::ListenerFuncVoid func_)
      : sender(sender_)
      , func(func_)
      , func2()
      , target()
    {}

    QueueItem(Valuable::Node * sender_, Valuable::Node * target_,
              const QByteArray & to_, const Radiant::BinaryData & data_)
      : sender(sender_)
      , func()
      , func2()
      , target(target_)
      , to(to_)
      , data(data_)
    {}

    Valuable::Node * sender;
    Valuable::Node::ListenerFuncVoid func;
    Valuable::Node::ListenerFuncBd func2;
    Valuable::Node * target;
    const QByteArray to;
    Radiant::BinaryData data;
  };

  // recursive because ~Node() might be called from processQueue()
  Radiant::Mutex s_queueMutex(true);
  std::list<std::unique_ptr<QueueItem> > s_queue;
  QSet<void *> s_queueOnce;

  Radiant::Mutex s_processingQueueMutex;
  bool s_processingQueue = false;
  std::list<std::unique_ptr<QueueItem> > s_queueTmp;
  QSet<void *> s_queueOnceTmp;

  void queueEvent(std::unique_ptr<QueueItem> item, void * once)
  {
    s_processingQueueMutex.lock();
    {
      if (s_processingQueue) {
        if (once) {
          if (s_queueOnceTmp.contains(once)) {
            s_processingQueueMutex.unlock();
            return;
          }
          s_queueOnceTmp << once;
        }
        s_queueTmp.push_back(std::move(item));
        s_processingQueueMutex.unlock();
        return;
      }
    }

    Radiant::Guard g(s_queueMutex);
    s_processingQueueMutex.unlock();

    if (once) {
      if (s_queueOnce.contains(once)) return;
      s_queueOnce << once;
    }
    s_queue.push_back(std::move(item));
  }

  void queueEvent(Valuable::Node * sender, Valuable::Node * target,
                  const QByteArray & to, const Radiant::BinaryData & data,
                  void * once)
  {
    queueEvent(std::unique_ptr<QueueItem>(new QueueItem(sender, target, to, data)), once);
  }

  void queueEvent(Valuable::Node * sender, Valuable::Node::ListenerFuncVoid func,
                  void * once)
  {
    queueEvent(std::unique_ptr<QueueItem>(new QueueItem(sender, func)), once);
  }

  void queueEvent(Valuable::Node * sender, Valuable::Node::ListenerFuncBd func,
                  const Radiant::BinaryData & data, void * once)
  {
    queueEvent(std::unique_ptr<QueueItem>(new QueueItem(sender, func, data)), once);
  }
}

namespace Valuable
{
#ifdef MULTI_DOCUMENTER
  VALUABLE_API std::map<QString, std::set<QString> > s_eventSendNames;
  VALUABLE_API std::map<QString, std::set<QString> > s_eventListenNames;
#endif

  static bool s_fatalOnEventMismatch = false;

  inline bool Node::ValuePass::operator == (const ValuePass & that) const
  {
    return (m_listener == that.m_listener) && (m_from == that.m_from) &&
           (m_to == that.m_to);
  }

  Node::Node()
      : Attribute(),
      m_sender(nullptr),
      m_eventsEnabled(true),
      m_id(nullptr, "id", generateId()),
      m_frame(0),
      m_listenersId(0)
  {
    eventAddOut("attribute-added");
    eventAddOut("attribute-removed");
    addAttribute("id", &m_id);
  }

  Node::Node(Node * host, const QByteArray & name, bool transit)
      : Attribute(host, name, transit),
      m_sender(nullptr),
      m_eventsEnabled(true),
      m_id(nullptr, "id", generateId()),
      m_frame(0),
      m_listenersId(0)
  {
    eventAddOut("attribute-added");
    eventAddOut("attribute-removed");
    addAttribute("id", &m_id);
  }

  Node::~Node()
  {
    // Host of Node class member Attributes must be zeroed to avoid double-delete
    m_id.removeHost();

    while(!m_eventSources.empty()) {
      /* The eventRemoveListener call will also clear the relevant part from m_eventSources. */
      m_eventSources.begin()->first->eventRemoveListener(this);
    }

    for(Listeners::iterator it = m_elisteners.begin(); it != m_elisteners.end(); ++it) {

      if(it->m_listener)
        it->m_listener->eventRemoveSource(this);

    }

    foreach(Attribute* vo, m_attributeListening) {
      for(QMap<long, AttributeListener>::iterator it = vo->m_listeners.begin(); it != vo->m_listeners.end(); ) {
        if(it->listener == this) {
          it = vo->m_listeners.erase(it);
        } else ++it;
      }
    }

    {
      Radiant::Guard g(s_queueMutex);
      for(auto it = s_queue.begin(); it != s_queue.end(); ++it) {
        auto & item = *it;
        if(item->target == this)
          item->target = 0;
        if(item->sender == this)
          item->sender = 0;
      }
      for(auto it = s_queueTmp.begin(); it != s_queueTmp.end(); ++it) {
        auto & item = *it;
        if(item->target == this)
          item->target = 0;
        if(item->sender == this)
          item->sender = 0;
      }
    }

    // Release memory for any attributes that are left (should be only
    // heap-allocated at this point)
    while(!m_attributes.empty())
      delete m_attributes.begin()->second;
  }

  Node::Node(Node && node)
    : Attribute(std::move(*this))
    , m_sender(std::move(node.m_sender))
    , m_attributes(std::move(node.m_attributes))
    , m_elisteners(std::move(node.m_elisteners))
    , m_eventSources(std::move(node.m_eventSources))
    , m_eventsEnabled(std::move(node.m_eventsEnabled))
    , m_attributeListening(std::move(node.m_attributeListening))
    , m_id(nullptr, "id", node.m_id)
    , m_frame(std::move(node.m_frame))
    , m_listenersId(std::move(node.m_listenersId))
    , m_eventSendNames(std::move(node.m_eventSendNames))
    , m_eventListenNames(std::move(node.m_eventListenNames))
  {
    node.m_id.m_host = nullptr;
    m_id.m_host = this;
    m_attributes[m_id.name()] = &m_id;
  }

  Node & Node::operator=(Node && node)
  {
    Attribute::operator=(std::move(*this));
    m_sender = std::move(node.m_sender);
    m_attributes = std::move(node.m_attributes);
    m_elisteners = std::move(node.m_elisteners);
    m_eventSources = std::move(node.m_eventSources);
    m_eventsEnabled = std::move(node.m_eventsEnabled);
    m_attributeListening = std::move(node.m_attributeListening);
    m_frame = std::move(node.m_frame);
    m_listenersId = std::move(node.m_listenersId);
    m_eventSendNames = std::move(node.m_eventSendNames);
    m_eventListenNames = std::move(node.m_eventListenNames);

    node.m_id.m_host = nullptr;
    m_id = node.m_id.value();
    m_id.setAsDefaults();
    m_id.m_host = nullptr;
    m_id.setName("id");
    m_id.m_host = this;
    m_attributes[m_id.name()] = &m_id;
    return *this;
  }

  Attribute * Node::getValue(const QByteArray & name) const
  {
    return Node::attribute(name);
  }

  Attribute * Node::attribute(const QByteArray & name) const
  {
    size_t slashIndex = name.indexOf('/');

    if(slashIndex == std::string::npos) {
      container::const_iterator it = m_attributes.find(name);

      return it == m_attributes.end() ? 0 : it->second;
    }
    else {
      const QByteArray part1 = name.left(slashIndex);
      const QByteArray part2 = name.mid(slashIndex + 1);

      const Attribute * attr = attribute(part1);
      if(attr) {
        return attr->attribute(part2);
      }
    }

    return nullptr;
  }

  bool Node::addValue(Attribute * const value)
  {
    return Node::addAttribute(value);
  }

  bool Node::addAttribute(Attribute * const attribute)
  {
    return Node::addAttribute(attribute->name(), attribute);
  }

  bool Node::addValue(const QByteArray & cname, Attribute * const value)
  {
    return Node::addAttribute(cname, value);
  }

  bool Node::addAttribute(const QByteArray & cname, Attribute * const attribute)
  {
    //    Radiant::trace("Node::addValue # adding %s", cname.c_str());

    // Check attributes
    if(m_attributes.find(cname) != m_attributes.end()) {
      Radiant::error(
          "Node::addAttribute # can not add attribute '%s' as '%s' "
          "already has an attribute with the same name.",
          cname.data(), m_name.data());
      return false;
    }

    // Unlink host if necessary
    Node * host = attribute->host();
    if(host) {
      Radiant::error(
          "Node::addAttribute # '%s' already has a host '%s'. "
          "Unlinking it to set new host.",
          cname.data(), host->name().data());
      attribute->removeHost();
    }

    // Change the attribute name
    attribute->setName(cname);

    m_attributes[attribute->name()] = attribute;
    attribute->m_host  = this;
    eventSend("attribute-added", attribute->name());
    attributeAdded(attribute);

    return true;
  }

  void Node::removeValue(Attribute * const value)
  {
    Node::removeAttribute(value);
  }

  void Node::removeAttribute(Attribute * const attribute)
  {
    for (auto it = m_attributes.begin(), end = m_attributes.end(); it != end; ++it) {
      if (it->second == attribute) {
        m_attributes.erase(it);
        attribute->m_host = nullptr;
        eventSend("attribute-removed", attribute->name());
        attributeRemoved(attribute);
        return;
      }
    }

    Radiant::error("Node::removeAttribute # '%s' is not a child attribute of '%s'.",
                   attribute->name().data(), m_name.data());
  }

#ifdef CORNERSTONE_JS

  bool Node::setValue(const QByteArray & name, v8::Handle<v8::Value> v)
  {
    using namespace v8;
    HandleScope handle_scope;
    if (v.IsEmpty()) {
      Radiant::error("Node::setValue # v8::Handle is empty");
      return false;
    }

    if (v->IsTrue()) return setValue(name, 1);
    if (v->IsFalse()) return setValue(name, 0);
    if (v->IsBoolean()) return setValue(name, v->ToBoolean()->Value() ? 1 : 0);
    if (v->IsInt32()) return setValue(name, int(v->ToInt32()->Value()));
    if (v->IsUint32()) return setValue(name, int(v->ToUint32()->Value()));
    if (v->IsString()) return setValue(name, QString::fromUtf16(*String::Value(v->ToString())));
    if (v->IsNumber()) return setValue(name, float(v->ToNumber()->NumberValue()));

    if (v->IsArray()) {
      Handle<Array> arr = v.As<Array>();
      assert(!arr.IsEmpty());
      if(arr->Length() == 2) {
        if (arr->Get(0)->IsNumber() && arr->Get(1)->IsNumber()) {
          return setValue(name, Nimble::Vector2f(arr->Get(0)->ToNumber()->Value(),
                                                 arr->Get(1)->ToNumber()->Value()));
        } else {
          Radiant::error("Node::setValue # v8::Value should be array of two numbers");
          return false;
        }
      } else if(arr->Length() == 3) {
        if (arr->Get(0)->IsNumber() && arr->Get(1)->IsNumber() && arr->Get(2)->IsNumber()) {
          return setValue(name, Nimble::Vector3f(arr->Get(0)->ToNumber()->Value(),
                                                 arr->Get(1)->ToNumber()->Value(),
                                                 arr->Get(2)->ToNumber()->Value()));
        } else {
          Radiant::error("Node::setValue # v8::Value should be array of three numbers");
          return false;
        }
      } else if(arr->Length() == 4) {
        if (arr->Get(0)->IsNumber() && arr->Get(1)->IsNumber() &&
            arr->Get(2)->IsNumber() && arr->Get(3)->IsNumber()) {
          return setValue(name, Nimble::Vector4f(arr->Get(0)->ToNumber()->Value(),
                                                 arr->Get(1)->ToNumber()->Value(),
                                                 arr->Get(2)->ToNumber()->Value(),
                                                 arr->Get(3)->ToNumber()->Value()));
        } else {
          Radiant::error("Node::setValue # v8::Value should be array of four numbers");
          return false;
        }
      }
      Radiant::error("Node::setValue # v8::Array with %d elements is not supported", arr->Length());
    } else if (v->IsRegExp()) {
      Radiant::error("Node::setValue # v8::Value type RegExp is not supported");
    } else if (v->IsDate()) {
      Radiant::error("Node::setValue # v8::Value type Date is not supported");
    } else if (v->IsExternal()) {
      Radiant::error("Node::setValue # v8::Value type External is not supported");
    } else if (v->IsObject()) {
      Radiant::error("Node::setValue # v8::Value type Object is not supported");
    } else if (v->IsFunction()) {
      Radiant::error("Node::setValue # v8::Value type Function is not supported");
    } else if (v->IsNull()) {
      Radiant::error("Node::setValue # v8::Value type Null is not supported");
    } else if (v->IsUndefined()) {
      Radiant::error("Node::setValue # v8::Value type Undefined is not supported");
    } else {
      Radiant::error("Node::setValue # v8::Value type is unknown");
    }
    return false;
  }
#endif

  bool Node::saveToFileXML(const QString & filename, unsigned int opts) const
  {
    bool ok = Serializer::serializeXML(filename, this, opts);
    if (!ok) {
      Radiant::error("Node::saveToFileXML # object failed to serialize (%s)", filename.toUtf8().data());
    }
    return ok;
  }

  bool Node::saveToMemoryXML(QByteArray & buffer, unsigned int opts) const
  {
    XMLArchive archive(opts);
    archive.setRoot(serialize(archive));

    return archive.writeToMem(buffer);
  }

  bool Node::loadFromFileXML(const QString & filename)
  {
    XMLArchive archive;

    if(!archive.readFromFile(filename))
      return false;

    return deserialize(archive.root());
  }

  bool Node::loadFromMemoryXML(const QByteArray & buffer)
  {
    XMLArchive archive;

    if(!archive.readFromMem(buffer))
      return false;

    return deserialize(archive.root());
  }

  ArchiveElement Node::serialize(Archive & archive) const
  {
    QString name = m_name.isEmpty() ? "Node" : m_name;

    ArchiveElement elem = archive.createElement(name.toUtf8().data());
    if(elem.isNull()) {
      Radiant::error(
          "Node::serialize # failed to create element");
      return ArchiveElement();
    }

    elem.add("type", type());

    for(container::const_iterator it = m_attributes.begin(); it != m_attributes.end(); ++it) {
      Attribute * vo = it->second;

      /// @todo need to add new flag to Archive that controls how Attribute::isSerializable works
      if (!vo->isSerializable())
        continue;

      ArchiveElement child = vo->serialize(archive);
      if (!child.isNull())
        elem.add(child);
    }

    return elem;
  }

  bool Node::deserialize(const ArchiveElement & element)
  {
    // Name
    m_name = element.name().toUtf8();

    // Children
    for(ArchiveElement::Iterator it = element.children(); it; ++it) {
      ArchiveElement elem = *it;

      QByteArray name = elem.name().toUtf8();

      Attribute * vo = attribute(name);

      // If the attribute exists, just deserialize it. Otherwise, pass the element
      // to readElement()
      bool ok = false;
      if(vo)
        ok = vo->deserialize(elem);
      if(!ok)
        ok = readElement(elem);
      if(!ok) {
        Radiant::error(
            "Node::deserialize # (%s) don't know how to handle element '%s'", type().data(), name.data());
        return false;
      }
    }

    return true;
  }

  void Node::debugDump() {
    Radiant::trace(Radiant::DEBUG, "%s {", m_name.data());

    for(container::iterator it = m_attributes.begin(); it != m_attributes.end(); ++it) {
      Attribute * vo = it->second;

      Node * hv = dynamic_cast<Node *> (vo);
      if(hv) hv->debugDump();
      else {
        QString s = vo->asString();
        Radiant::trace(Radiant::DEBUG, "\t%s = %s", vo->name().data(), s.toUtf8().data());
      }
    }

    Radiant::trace(Radiant::DEBUG, "}");
  }

  long Node::eventAddListener(const QByteArray & fromIn,
                              const QByteArray & to,
                              Valuable::Node * obj,
                              ListenerType listenerType,
                              const Radiant::BinaryData * defaultData)
  {
    const QByteArray from = validateEvent(fromIn);

    ValuePass vp(++m_listenersId);
    vp.m_listener = obj;
    vp.m_from = from;
    vp.m_to = to;
    vp.m_frame = m_frame;
    vp.m_type = listenerType;

    if(!obj->m_eventListenNames.contains(to)) {
      if(!obj->attribute(to)) {
        /* If the to attribute is not a known listener, or an attribute we output a warning.
          */
        /** @todo We could still check that the "to" is not a hierarchical path, for example
           "widget1/color".
        */

        const QByteArray & klass = Radiant::StringUtils::demangle(typeid(*obj).name());
        if(s_fatalOnEventMismatch)
          Radiant::fatal("Node::eventAddListener # %s (%s %p) doesn't accept event '%s'",
                           klass.data(), obj->name().data(), obj, to.data());
        else
          Radiant::warning("Node::eventAddListener # %s (%s %p) doesn't accept event '%s'",
                           klass.data(), obj->name().data(), obj, to.data());
      }
    }

    if(defaultData)
      vp.m_defaultData = *defaultData;

    if(std::find(m_elisteners.begin(), m_elisteners.end(), vp) !=
       m_elisteners.end())
      debugValuable("Widget::eventAddListener # Already got item %s -> %s (%p)",
            from.data(), to.data(), obj);
    else {
      m_elisteners.push_back(vp);
      obj->eventAddSource(this);
    }
    return vp.m_listenerId;
  }

  long Node::eventAddListener(const QByteArray & fromIn, ListenerFuncVoid func,
                              ListenerType listenerType)
  {
    const QByteArray from = validateEvent(fromIn);

    ValuePass vp(++m_listenersId);
    vp.m_func = func;
    vp.m_from = from;
    vp.m_type = listenerType;

    // No duplicate check, since there is no way to compare std::function objects
    m_elisteners.push_back(vp);
    return vp.m_listenerId;
  }

  long Node::eventAddListenerBd(const QByteArray & fromIn, ListenerFuncBd func,
                                ListenerType listenerType)
  {
    const QByteArray from = validateEvent(fromIn);

    ValuePass vp(++m_listenersId);
    vp.m_func2 = func;
    vp.m_from = from;
    vp.m_type = listenerType;

    // No duplicate check, since there is no way to compare std::function objects
    m_elisteners.push_back(vp);
    return vp.m_listenerId;
  }

  int Node::eventRemoveListener(const QByteArray & from, const QByteArray & to, Valuable::Node * obj)
  {
    int removed = 0;

    for(Listeners::iterator it = m_elisteners.begin(); it != m_elisteners.end(); ) {

      // match obj if specified
      if(!obj || it->m_listener == obj) {
        // match from & to if specified
        if((from.isNull() || it->m_from == from) &&
           (to.isNull() || it->m_to == to)) {

          if (it->m_listener)
            it->m_listener->eventRemoveSource(this);

          it = m_elisteners.erase(it);
          removed++;
          continue;
        }
      }
      ++it;
    }

    return removed;
  }

  bool Node::eventRemoveListener(long listenerId)
  {
    for (auto it = m_elisteners.begin(); it != m_elisteners.end(); ++it) {
      if (it->m_listenerId == listenerId) {
        if (it->m_listener)
          it->m_listener->eventRemoveSource(this);
        it = m_elisteners.erase(it);
        return true;
      }
    }
    return false;
  }

  void Node::attributeAdded(Attribute *)
  {
  }

  void Node::attributeRemoved(Attribute *)
  {
  }

  void Node::setFatalOnEventMismatch(bool haltApplication)
  {
    s_fatalOnEventMismatch = haltApplication;
  }

  void Node::eventAddSource(Valuable::Node * source)
  {
    ++m_eventSources[source];
  }

  void Node::eventRemoveSource(Valuable::Node * source)
  {
    Sources::iterator it = m_eventSources.find(source);

    if (it != m_eventSources.end() && --it->second <= 0)
      m_eventSources.erase(it);
  }

  void Node::eventProcess(const QByteArray & id, Radiant::BinaryData & data)
  {
    // Radiant::info("Node::eventProcess # %s %s", typeid(*this).name(), id);

    int idx = id.indexOf('/');
    QByteArray n = idx == -1 ? id : id.left(idx);

    // Radiant::info("Node::eventProcess # Child id = %s", key.c_str());

    Attribute * vo = attribute(n);

    if(vo) {
      // Radiant::info("Node::eventProcess # Sending message \"%s\" to %s",
      // id + skip, typeid(*vo).name());
      vo->eventProcess(idx == -1 ? "" : id.mid(idx + 1), data);
    } else {
      if(!m_eventListenNames.contains(n)) {
        /*warning("Node::eventProcess # %s (%s %p) doesn't accept event '%s'",
                  klass.c_str(), name().c_str(), this, id);*/
      } else {
        const QByteArray klass = Radiant::StringUtils::demangle(typeid(*this).name());
        Radiant::warning("Node::eventProcess # %s (%s %p): unhandled event '%s'",
                klass.data(), name().data(), this, id.data());
      }
    }
  }

  // Must be outside function definition to be thread-safe
  static Radiant::Mutex s_generateIdMutex;

  Node::Uuid Node::generateId()
  {
    Radiant::Guard g(s_generateIdMutex);
    static Uuid s_id = static_cast<Uuid>(Radiant::TimeStamp::currentTime().value());
    return s_id++;
  }

  Node::Uuid Node::id() const
  {
    return m_id;
  }

  void Node::eventAddOut(const QByteArray & id)
  {
    if (m_eventSendNames.contains(id)) {
      Radiant::warning("Node::eventAddOut # Trying to register event '%s' that is already registered", id.data());
    } else {
      m_eventSendNames.insert(id);
#ifdef MULTI_DOCUMENTER
      s_eventSendNames[Radiant::StringUtils::demangle(typeid(*this).name())].insert(id);
#endif
    }
  }

  void Node::eventAddIn(const QByteArray &id)
  {
    if (m_eventListenNames.contains(id)) {
      Radiant::warning("Node::eventAddIn # Trying to register duplicate event handler for event '%s'", id.data());
    } else {
      m_eventListenNames.insert(id);
#ifdef MULTI_DOCUMENTER
      s_eventListenNames[Radiant::StringUtils::demangle(typeid(*this).name())].insert(id);
#endif
    }
  }

  void Node::eventRemoveOut(const QByteArray & eventId)
  {
    if (m_eventSendNames.contains(eventId)) {
      m_eventSendNames.remove(eventId);
    } else {
      Radiant::warning("Node::eventRemoveOut # Couldn't find event '%s'", eventId.data());
    }

  }

  void Node::eventRemoveIn(const QByteArray & messageId)
  {
    if (m_eventListenNames.contains(messageId)) {
      m_eventListenNames.remove(messageId);
    } else {
      Radiant::warning("Node::eventRemoveIn # Couldn't find event '%s'", messageId.data());
    }
  }

  bool Node::acceptsEvent(const QByteArray & id) const
  {
    return m_eventListenNames.contains(id);
  }

#ifdef CORNERSTONE_JS
  long Node::addListener(const QByteArray & name, v8::Persistent<v8::Function> func, int role)
  {
    Attribute * attr = attribute(name);
    if(!attr) {
      Radiant::warning("Node::addListener # Failed to find attribute %s", name.data());
      return -1;
    }
    return attr->addListener(func, role);
  }
#endif
  int Node::processQueue()
  {
    {
      Radiant::Guard g(s_processingQueueMutex);
      s_processingQueue = true;
    }

    /// The queue must be locked during the whole time when calling the callback
    Radiant::Guard g(s_queueMutex);

    // Can not use range-based loop here because it doesn't iterate all
    // elements when the QList gets modified inside the loop.
    for(auto i = s_queue.begin(); i != s_queue.end(); ++i) {
      auto & item = *i;
      if(item->target) {
        std::swap(item->target->m_sender, item->sender);
        item->target->eventProcess(item->to, item->data);
        std::swap(item->target->m_sender, item->sender);
      } else if(item->func) {
        item->func();
      } else if(item->func2) {
        item->func2(item->data);
      }
      // can't call "delete item" here, because that eventProcess call could
      // call some destructors that iterate s_queue
    }

    int r = s_queue.size();

    {
      // Make a temporary copy to prevent weird callback recursion bugs
      auto tempQueue = std::move(s_queue);
      s_queue.clear();
    }

    // Since we are locking two mutexes at the same time also in queueEvent,
    // it's important that the lock order is right. Always lock
    // s_processingQueueMutex before s_queueMutex. That is why we need to
    // release the lock, otherwise we will get deadlock if queueEvent has
    // already locked s_processingQueueMutex and is waiting for s_queueMutex.
    // Also remember to clear s_queue, otherwise ~Node() could be reading old
    // deleted values from it
    s_queueMutex.unlock();
    Radiant::Guard g2(s_processingQueueMutex);
    s_queueMutex.lock();

    s_queue = std::move(s_queueTmp);
    s_queueOnce = s_queueOnceTmp;
    s_queueTmp.clear();
    s_queueOnceTmp.clear();
    s_processingQueue = false;
    return r;
  }

  bool Node::copyValues(const Node & from, Node & to)
  {
    XMLArchive archive;
    ArchiveElement e = Valuable::Serializer::serialize(archive, from);
    Uuid toId = to.id();

    if(!e.isNull()) {
      bool ok = to.deserialize(e);
      to.m_id = toId;
      return ok;
    }

    return false;
  }

  void Node::invokeAfterUpdate(Node::ListenerFuncVoid function)
  {
    queueEvent(nullptr, function, nullptr);
  }

  void Node::eventSend(const QByteArray & id, Radiant::BinaryData & bd)
  {
    if(!m_eventsEnabled)
      return;

    if(!m_eventSendNames.contains(id)) {
      Radiant::error("Node::eventSend # Sending unknown event '%s'", id.data());
    }

    m_frame++;

    auto listenerCopy = m_elisteners;

    for(Listeners::iterator it = listenerCopy.begin(); it != listenerCopy.end();) {
      ValuePass & vp = *it;

      if(vp.m_frame == m_frame) {
        /* The listener was added during this function call. Lets not call it yet. */
      }
      else if(vp.m_from == id) {

        Radiant::BinaryData & bdsend = vp.m_defaultData.total() ? vp.m_defaultData : bd;

        bdsend.rewind();

        if(vp.m_listener) {
          if(vp.m_type == AFTER_UPDATE_ONCE) {
            queueEvent(this, vp.m_listener, vp.m_to, bdsend, &vp);
          } else if(vp.m_type == AFTER_UPDATE) {
            queueEvent(this, vp.m_listener, vp.m_to, bdsend, 0);
          } else {
            // m_sender is valid only at the beginning of eventProcess call
            Node * sender = this;
            std::swap(vp.m_listener->m_sender, sender);
            vp.m_listener->eventProcess(vp.m_to, bdsend);
            vp.m_listener->m_sender = sender;
          }
        } else if(vp.m_func) {
          if(vp.m_type == AFTER_UPDATE_ONCE) {
            queueEvent(this, vp.m_func, &vp);
          } else if(vp.m_type == AFTER_UPDATE) {
            queueEvent(this, vp.m_func, 0);
          } else {
            vp.m_func();
          }
        } else if(vp.m_func2) {
          if(vp.m_type == AFTER_UPDATE_ONCE) {
            queueEvent(this, vp.m_func2, bdsend, &vp);
          } else if(vp.m_type == AFTER_UPDATE) {
            queueEvent(this, vp.m_func2, bdsend, 0);
          } else {
            vp.m_func2(bdsend);
          }
        }
      }
      ++it;
    }
  }

  void Node::eventSend(const QByteArray & id)
  {
    Radiant::BinaryData tmp;
    eventSend(id, tmp);
  }

  void Node::attributeRenamed(const QByteArray & was, const QByteArray & now)
  {
    // Check that the attribute does not exist already
    iterator it = m_attributes.find(now);
    if(it != m_attributes.end()) {
      Radiant::error("Node::attributeRenamed # Attribute '%s' already exist", now.data());
      return;
    }

    it = m_attributes.find(was);
    if(it == m_attributes.end()) {
      Radiant::error("Node::attributeRenamed # No such attribute: %s", was.data());
      return;
    }

    Attribute * vo = (*it).second;
    m_attributes.erase(it);
    m_attributes[now] = vo;
  }

  bool Node::readElement(const ArchiveElement &)
  {
    return false;
  }

  void Node::clearValues(Layer layer)
  {
    for(auto i = m_attributes.begin(); i != m_attributes.end(); ++i)
      i->second->clearValue(layer);
  }

  void Node::setAsDefaults()
  {
    for(auto i = m_attributes.begin(); i != m_attributes.end(); ++i)
      i->second->setAsDefaults();
  }

  bool Node::isChanged() const
  {
    for (auto i = m_attributes.begin(); i != m_attributes.end(); ++i)
      if (i->second->isChanged())
        return true;
    return false;
  }

  void Node::eventAddDeprecated(const QByteArray &deprecatedId, const QByteArray &newId)
  {
    m_deprecatedEventCompatibility[deprecatedId] = newId;
  }

  QByteArray Node::validateEvent(const QByteArray &from)
  {
    // Issue warning if the original requested event is not registered
    if(!m_eventSendNames.contains(from)) {

      // Check for event conversions
      if(m_deprecatedEventCompatibility.contains(from)) {

        const QByteArray & converted = m_deprecatedEventCompatibility[from];
        Radiant::warning("The event '%s' is deprecated. Use '%s' instead.", from.data(), converted.data());

        return converted;
      }

      if(s_fatalOnEventMismatch)
        Radiant::fatal("Node::validateEvent # event '%s' does not exist for this class", from.data());
      else
        Radiant::warning("Node::validateEvent # event '%s' does not exist for this class", from.data());

    }

    return from;
  }

}
