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
#if !defined (RADIANT_MEMORY_HPP)
#define RADIANT_MEMORY_HPP

#include <Radiant/Platform.hpp>
#include <memory>

namespace Radiant
{
// Some memory utility functions
#if defined (RADIANT_WINDOWS)
   inline void * AlignedMalloc(size_t size, unsigned int alignment)
   {
     void * ptr = _aligned_malloc(size, alignment);
     if (ptr == 0)
       throw std::bad_alloc();
     return ptr;
   }

   inline void AlignedFree(void * ptr) { _aligned_free(ptr); }

   template<typename T>
   inline T * AddressOf(T & rhs) { return std::addressof(rhs); }

#elif defined (RADIANT_UNIX)
   inline void * AlignedMalloc(size_t size, unsigned int alignment)
   {
      void *ptr = 0;
      int r = posix_memalign(&ptr, alignment, size);
      if (r != 0)
            throw std::bad_alloc();
      return ptr;
   }
   inline void AlignedFree(void * ptr) { ::free(ptr); }

   template<typename T>
   inline T * AddressOf(T & rhs) { return std::__addressof(rhs); }
#endif
}

#endif // RADIANT_MEMORY_HPP