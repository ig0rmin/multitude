/* COPYRIGHT
 *
 * This file is part of Radiant.
 *
 * Copyright: MultiTouch Oy, Helsinki University of Technology and others, 2007-2013
 *
 * This file is licensed under GNU Lesser General Public
 * License (LGPL), version 2.1. The LGPL conditions can be found in 
 * file "LGPL.txt" that is distributed with this source package or obtained 
 * from the GNU organization (www.gnu.org).
 * 
 */

#if !defined (RADIANT_CALLSTACK_HPP)
#define RADIANT_CALLSTACK_HPP

#include "Export.hpp"
#include "Platform.hpp"

#include <iostream>
#include <cassert>
#include <cstdint>

#ifdef RADIANT_WINDOWS
  typedef uint64_t stackptr_t;
#else
  typedef void * stackptr_t;
#endif

namespace Radiant
{
  /// Captures the current callstack
  class CallStack
  {
  public:

    RADIANT_API CallStack();
    RADIANT_API ~CallStack();

    /// @returns the raw callstack
    const stackptr_t * stack() const { return m_frames; }

    /// @param index The requested element in the callstack
    /// @returns the requested element in the callstack
    stackptr_t operator[](size_t index) const { assert(index < m_frameCount); return m_frames[index]; }

    /// @returns the number of frames in the callstack
    size_t size() const { return m_frameCount; }

    /// Prints a human-readable version of the stack to the log
    RADIANT_API void print() const;

  private:
    enum { max_frames = 32 };
    stackptr_t m_frames[max_frames];
    size_t m_frameCount;
  };
}

#endif
