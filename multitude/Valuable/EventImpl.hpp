namespace Valuable
{
  /// VS2017 doesn't support TLS variables at DLL interface, but using this
  /// workaround getter function on Linux causes ~7% time increase in
  /// ValuableEventTestSimple benchmark, so we use the faster implementation
  /// on Linux since we can.
#ifdef RADIANT_WINDOWS
  VALUABLE_API uint32_t & removeCurrentEventListenerCounter();
#else
  VALUABLE_API extern thread_local uint32_t t_removeCurrentEventListenerCounter;
#define removeCurrentEventListenerCounter() t_removeCurrentEventListenerCounter
#endif

  template <typename... Args>
  struct Event<Args...>::Listener
  {
    Listener(int id_, Callback callback_, folly::Executor * executor_, Valuable::Node * receiver_, EventFlags flags_)
      : executor(executor_)
      , receiver(receiver_)
      , id(id_)
      , flags(flags_)
      , hasReceiver(receiver_ != nullptr)
      , callback(std::move(callback_))
    {}

    Listener(Listener && l)
      : executor(l.executor)
      , receiver(std::move(l.receiver))
      , id(l.id)
      , flags(l.flags)
      , hasReceiver(l.hasReceiver)
      , valid(l.valid.load())
      , callback(std::move(l.callback))
    {}

    Listener & operator=(Listener && l)
    {
      executor = l.executor;
      receiver = std::move(l.receiver);
      id = l.id;
      flags = l.flags;
      hasReceiver = l.hasReceiver;
      valid = l.valid.load();
      callback = std::move(l.callback);
      return *this;
    }

    folly::Executor * executor;
    Valuable::WeakNodePtrT<Valuable::Node> receiver;
    int id;
    EventFlags flags;
    bool hasReceiver;
    // Set to false if removeListener was called while the event was raised
    std::atomic<bool> valid{true};
    Callback callback;
  };

  template <typename... Args>
  struct Event<Args...>::D
  {
    /// Creates m_d if needed
    static Event<Args...>::D & get(Event<Args...> & e)
    {
      auto d = e.m_d.load();
      if (d)
        return *d;
      auto d2 = new typename Event<Args...>::D();
      if (e.m_d.compare_exchange_strong(d, d2)) {
        return *d2;
      } else {
        // someone else created m_d faster than us
        delete d2;
        d = e.m_d.load();
        assert(d);
        return *d;
      }
    }

    /// This mutex protects all member variables here. Additionally if 'raising'
    /// is non-zero, 'listeners' can be read and their atomic member variables
    /// can be also written.
    QMutex mutex;
    std::vector<Listener> listeners;
    std::vector<Listener> newListeners;
    int raising = 0;
    int nextId = 0;

    bool addedDuringRaise = false;
    bool removedDuringRaise = false;
  };


  template <typename... Args>
  Event<Args...>::~Event()
  {
    delete m_d.load();
  }

  template <typename... Args>
  int Event<Args...>::addListener(Callback callback)
  {
    return addListener(EventFlag::NO_FLAGS, nullptr, nullptr, std::move(callback));
  }

  template <typename... Args>
  int Event<Args...>::addListener(Valuable::Node * receiver, Callback callback)
  {
    return addListener(EventFlag::NO_FLAGS, receiver, nullptr, std::move(callback));
  }

  template <typename... Args>
  int Event<Args...>::addListener(folly::Executor * executor, Callback callback)
  {
    return addListener(EventFlag::NO_FLAGS, nullptr, executor, std::move(callback));
  }

  template <typename... Args>
  int Event<Args...>::addListener(EventFlags flags, Callback callback)
  {
    return addListener(flags, nullptr, nullptr, std::move(callback));
  }

  template <typename... Args>
  int Event<Args...>::addListener(EventFlags flags, Valuable::Node * receiver, folly::Executor * executor, Callback callback)
  {
    D & d = D::get(*this);
    QMutexLocker g(&d.mutex);
    int id = d.nextId++;
    if (d.raising) {
      d.newListeners.emplace_back(id, std::move(callback), executor, receiver, flags);
      d.addedDuringRaise = true;
    } else {
      d.listeners.emplace_back(id, std::move(callback), executor, receiver, flags);
    }
    return id;
  }

  template <typename... Args>
  void Event<Args...>::removeCurrentListener()
  {
    ++removeCurrentEventListenerCounter();
  }

  template <typename... Args>
  bool Event<Args...>::removeListener(int id)
  {
    D * d = m_d.load();
    if (!d)
      return false;

    // Do not delete the callback function while holding the lock
    Callback deleted;
    {
      QMutexLocker g(&d->mutex);
      for (auto it = d->listeners.begin(); it != d->listeners.end(); ++it) {
        if (it->id == id) {
          if (d->raising) {
            it->valid = false;
            d->removedDuringRaise = true;
          } else {
            deleted = std::move(it->callback);
            d->listeners.erase(it);
          }
          return true;
        }
      }

      for (auto it = d->newListeners.begin(); it != d->newListeners.end(); ++it) {
        if (it->id == id) {
          deleted = std::move(it->callback);
          d->newListeners.erase(it);
          return true;
        }
      }
    }

    return false;
  }

  template <typename... Args>
  void Event<Args...>::raise(Args... args)
  {
    D * d = m_d.load();
    if (!d)
      return;

    QMutexLocker g(&d->mutex);
    ++d->raising;

    /// Unlock the mutex while raising the event. We have set 'raising' to non-zero,
    /// so listeners is not modified, and all writes happen to tls or atomic
    /// variables. Calling raise / addListener recursively from a listener callback
    /// is also safe.
    d->mutex.unlock();

    bool removed = false;
    for (Listener & l: d->listeners) {
      if (l.flags & EventFlag::SINGLE_SHOT) {
        if (l.valid.exchange(false) == false)
          continue;
        removed = true;
      } else if (!l.valid)
        continue;

      uint32_t & counter = removeCurrentEventListenerCounter();
      uint32_t beforeCounter = counter;
      if (l.executor) {
        if (l.hasReceiver) {
          if (*l.receiver) {
            l.executor->add([receiver = l.receiver, callback = l.callback, args...] {
              if (*receiver)
                callback(args...);
            });
          } else {
            removed = true;
            l.valid = false;
          }
        } else {
          l.executor->add(std::bind(l.callback, args...));
        }
      } else {
        if (l.hasReceiver) {
          if (*l.receiver) {
            l.callback(args...);
          } else {
            removed = true;
            l.valid = false;
          }
        } else {
          l.callback(args...);
        }
      }

      if (counter != beforeCounter) {
        removed = true;
        l.valid = false;
        counter = beforeCounter;
      }
    }

    d->mutex.lock();
    if (removed && !d->removedDuringRaise)
      d->removedDuringRaise = true;

    // If there are more than one thread busy-looping raise(), it's possible
    // that 'raising' never reaches zero and new listeners won't get added.
    // That's not expected use case for this class, so we ignore the problem.
    if (--d->raising == 0) {
      if (d->removedDuringRaise) {
        d->removedDuringRaise = false;
        for (auto it = d->listeners.begin(); it != d->listeners.end();) {
          if (it->valid)
            ++it;
          else
            it = d->listeners.erase(it);
        }
      }

      if (d->addedDuringRaise) {
        d->addedDuringRaise = false;
        d->listeners.insert(d->listeners.end(),
                            std::make_move_iterator(d->newListeners.begin()),
                            std::make_move_iterator(d->newListeners.end()));
        d->newListeners.clear();
      }
    }
  }

  template <typename... Args>
  uint32_t Event<Args...>::listenerCount() const
  {
    D * d = m_d.load();
    if (!d)
      return 0;

    uint32_t count = 0;
    QMutexLocker g(&d->mutex);
    for (const Listener & l: d->listeners)
      if (l.valid)
        ++count;
    count += d->newListeners.size();
    return count;
  }

  template <typename... Args>
  Event<Args...>::Event(Event && e)
    : m_d(e.m_d.exchange(nullptr))
  {}

  template <typename... Args>
  Event<Args...> & Event<Args...>::operator=(Event && e)
  {
    D * old = m_d.exchange(nullptr);
    m_d = e.m_d.exchange(nullptr);
    delete old;
    return *this;
  }
}
