/* COPYRIGHT
 *
 * This file is part of Radiant.
 *
 * Copyright: MultiTouch Oy, Helsinki University of Technology and others.
 *
 * See file "Radiant.hpp" for authors and more details.
 *
 * This file is licensed under GNU Lesser General Public
 * License (LGPL), version 2.1. The LGPL conditions can be found in
 * file "LGPL.txt" that is distributed with this source package or obtained
 * from the GNU organization (www.gnu.org).
 *
 */

#ifndef RADIANT_THREAD_HPP
#define RADIANT_THREAD_HPP

#include <Radiant/Export.hpp>
#include <Radiant/Platform.hpp>
#include <Radiant/Mutex.hpp>

#include <Patterns/NotCopyable.hpp>

#include <QString>

#include <cstring>
#include <map>
#include <list>

class QThread;


namespace Radiant {

  class Mutex;
  /// Platform-independent threading
  /** This class is used by inheriting it and overriding the virtual
      method childLoop().
      */
  class RADIANT_API Thread : public Patterns::NotCopyable
  {
  public:

    /// Thread id type.
    /** On most systems this is some kind of integer value. */
    typedef void* id_t;

    /// The id of the calling thread
    static id_t myThreadId();

    /// Construct a thread structure. The thread is NOT activated by this
    /// method.
    /// @param name thread name
    /// @sa setName
    Thread(const QString & name = "Radiant::Thread");

    /// Destructor
    /** The thread must be stopped before this method is
    called. Thread cannot be terminated within the destructor, as
    the inheriting class that implements the virtual childLoop
    function does not exist any more (its destructor is called
    before this function). */
    virtual ~Thread();

    /// Set the thread name. The thread name can be used by some debuggers, for
    /// example QtCreator. Useful for debugging purposes. Does not affect any
    /// functionality.
    /// @param name thread name
    void setName(const QString &name);

    /** Starts the thread */
    void run();

    /** Waits until thread is finished. This method does nothing to
    kill the thread, it simply waits until the thread has run its
    course. */
    /// @param timeoutms Time to wait, in milliseconds
    /// @returns true if the thread has terminated within the timeout period
    bool waitEnd(int timeoutms = 0);

    /** Kills the thread. A violent way to shut down a thread. You
    should only call this method in emergency situations. May result
    in application crash and other minor problems.*/
    void kill();

    /// Check if the thread is running
    /// @returns true if the thread is running.
    bool isRunning() const;

    /** Drive some self tests. */
    //static void test();

  protected:
    /// Exits the the calling thread.
    void threadExit();

    /// Calls childLoop.
    void mainLoop();

    /** The actual contents of the thread. You need to override this to
    add functionality to your software. */
    virtual void childLoop() = 0;

  private:
    static void *entry(void *);

    class D;
    D * m_d;

    volatile int m_state;

    static bool m_threadDebug;
    static bool m_threadWarnings;
  };

  /// Thread Local Storage implementation.
  /// Do something like Radiant::TLS<int> foo = 5; and after that you can just use the foo as int
  template <typename T>
  class TLS
  {
    typedef std::map<Thread::id_t, T> Map;

  public:
    TLS() {}
    /// Construct a TLS variable with the given default value
    /// @param t default value
    TLS(const T& t) : m_default(t) {}
    /// Construct a copy
    /// @param t object to copy
    TLS(const  TLS & t)
    {
      Radiant::Guard g1(m_mutex);
      Radiant::Guard g2(m_mutex);
      m_default = t.m_default;
      m_values = t.m_values;
    }

    /// Get the calling thread instance of the TLS variable
    /// @return variable instance in calling thread
    T& get()
    {
      Thread::id_t id = Thread::myThreadId();
      Radiant::Guard g(m_mutex);
      typename Map::iterator it = m_values.find(id);
      if(it == m_values.end()) {
        m_values[id] = m_default;
        return m_values[id];
      }
      return it->second;
    }

    /// @copydoc get
    operator T&() { return get(); }

    /// Set all instances of the variable to the given value
    /// @param t value to set
    void setAll(const T & t)
    {
      Radiant::Guard g(m_mutex);
      m_default = t;
      typename Map::iterator it = m_values.begin(), it2 = m_values.end();
      while(it != it2) {
        it->second = t;
        ++it;
      }
    }

    /// Get all instances of the variable
    /// Returns a list of all instances of the TLS variable from different threads.
    /// @return list of all instances in different threads
    std::list<T> all() const
    {
      std::list<T> lst;
      Radiant::Guard g(m_mutex);
      typename Map::const_iterator it = m_values.begin(), it2 = m_values.end();
      while(it != it2) {
        lst.push_back(it->second);
        ++it;
      }
      return lst;
    }

    /// Assign the underlying value
    TLS<T> & operator=(const T& t)
    {
      get() = t;
      return *this;
    }

  private:
    T m_default;
    Map m_values;
    mutable Radiant::Mutex m_mutex;
  };

#if defined(RADIANT_LINUX)
  #define RADIANT_TLS(type) __thread type
#elif defined(RADIANT_WINDOWS)
  #define RADIANT_TLS(type) __declspec(thread) type
#else
  #define RADIANT_TLS(type) Radiant::TLS<type>
#endif

}

#endif
