/* Copyright (C) 2007-2013: Multi Touch Oy, Helsinki University of Technology
 * and others.
 *
 * This file is licensed under GNU Lesser General Public License (LGPL),
 * version 2.1. The LGPL conditions can be found in file "LGPL.txt" that is
 * distributed with this source package or obtained from the GNU organization
 * (www.gnu.org).
 * 
 */

#ifndef RADIANT_WATCHDOG_HPP
#define RADIANT_WATCHDOG_HPP

#include <Radiant/Export.hpp>
#include <Radiant/Singleton.hpp>
#include <Radiant/Mutex.hpp>
#include <Radiant/Thread.hpp>

#include <map>

#include <QByteArray>

namespace Radiant {

  /** A guard that is used to make sure that programs do not get stuck.

      If the program appears to be stuck (not calling @ref hostIsAlive for
      given time) then this class simply shuts down the application.
   */
  class RADIANT_API WatchDog FINAL : private Radiant::Thread
  {
    DECLARE_SINGLETON(WatchDog);

  public:
    /// Constructor
    WatchDog();
    /// Destructor
    ~WatchDog();

    /** Inform the watchdog that the host application is working. You can call this function
        at any time, and the call is fully thread-safe. After calling this method for the first time,
        you need to keep calling it periodically, to make sure that the watchdog knows you are there.

        @param key The identifier of the calling object. This is usually a point to some object
        which provides a handy way of generating unique keys in C/C++.
    */
    void hostIsAlive(void * key, const QByteArray & name);
    /** Instructs the Watchdog to forget some hosting object.
        @param key The identifier of the calling object.
    */
    void forgetHost(void * key);

    /// Sets the interval for checking if the host is alive.
    /// @param seconds Length of the interval in seconds
    void setInterval(float seconds) { m_intervalSeconds = seconds; }
    
    /** Stops the watchdog. */
    void stop();

    /// Pauses the watchdog
    void pause() { m_paused = true; }
    /// Unpause the watchdog
    void unpause() { m_paused = false; }

    /// Check if the watchdog is paused
    /// @return true if paused; otherwise false
    bool paused() const { return m_paused; }

  private:

    virtual void childLoop();

    class Item
    {
    public:
      Item() : m_check(true) {}
      volatile bool m_check;
      QByteArray m_name;
    };

    typedef std::map<void *, Item> container;

    container  m_items;

    Radiant::Mutex m_mutex;
    volatile bool m_continue;
    volatile float m_intervalSeconds;
    volatile bool m_paused;
  };

}

#endif
