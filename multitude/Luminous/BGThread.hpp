/* COPYRIGHT
 *
 * This file is part of Luminous.
 *
 * Copyright: MultiTouch Oy, Helsinki University of Technology and others.
 *
 * See file "Luminous.hpp" for authors and more details.
 *
 * This file is licensed under GNU Lesser General Public
 * License (LGPL), version 2.1. The LGPL conditions can be found in 
 * file "LGPL.txt" that is distributed with this source package or obtained 
 * from the GNU organization (www.gnu.org).
 * 
 */

#ifndef LUMINOUS_BGTHREAD_HPP
#define LUMINOUS_BGTHREAD_HPP

#include <Luminous/Export.hpp>
#include <Luminous/Task.hpp>

#include <Radiant/Condition.hpp>
#include <Radiant/Mutex.hpp>
#include <Radiant/Singleton.hpp>
#include <Radiant/ThreadPool.hpp>
#include <Radiant/RefPtr.hpp>

#include <list>
#include <map>
#include <set>

namespace Luminous
{

  /// A class used to execute tasks in a separated threads.

  class LUMINOUS_API BGThread : public Radiant::ThreadPool
  {
    DECLARE_SINGLETON(BGThread);
  public:
    BGThread();
    virtual ~BGThread();

    /// Add a task to be executed
    /** The task is the property of the BGThread, which will delete the object when its
        operation is finished and the shared pointer's reference count goes to zero.

        @param task The task that needs to be added.
    */
    virtual void addTask(std::shared_ptr<Task> task);

    /// Add a task to be executed
    /** The task is the property of the BGThread, which will delete the object when its
        operation is finished and the pointer's reference count goes to zero.

        @param task The task that needs to be added.
    */
    virtual void addTask(Task * task);

    /// Remove the task from the BGThread
    /** If you just want to delete the task, then it is generally better to set the state of
        the task to finished, and schedule it immediately for processing (and thus removal).

        @param task The task to be removed
        @return True if the task was successfully removes, false otherwise.
    */
    virtual bool removeTask(std::shared_ptr<Task> task);

    /// Update the changed task timestamp to queue
    virtual void reschedule(std::shared_ptr<Task> task);
    void reschedule(std::shared_ptr<Task> task, Priority p);


    /// Change the priority of a task
    virtual void setPriority(std::shared_ptr<Task> task, Priority p);

    /// Container for the tasks
    typedef std::multimap<Priority, std::shared_ptr<Task>, std::greater<Priority> > container;
    /// Objects stored in the task container
    typedef std::pair<Priority, std::shared_ptr<Task> > contained;

    /// Returns the number of tasks in the BGThread.
    unsigned taskCount();

    /// Get the number of tasks right now in doTask().
    /// This function is lock-free and O(1).
    unsigned int runningTasks() const;

    /// Get the number of tasks that should be running right now but are not
    /// yet processed. This function is slow: O(N), needs a mutex lock and
    /// calls TimeStamp::getTime().
    unsigned int overdueTasks() const;

    /// Dump information about the tasks at hand
    void dumpInfo(FILE * f = 0, int indent = 0);
  private:
    virtual void childLoop();

    std::shared_ptr<Task> pickNextTask();

    container::iterator findTask(std::shared_ptr<Task> task);

    void wakeThread();
    void wakeAll();

    // locked with m_mutexWait
    container m_taskQueue;

    // a thread is already waiting for these tasks
    std::set<std::shared_ptr<Task> > m_reserved;

    // number of idle threads, excluding ones that are reserving a task
    int m_idle;
    Radiant::Condition m_idleWait;

    QAtomicInt m_runningTasks;
  };

}

#endif

