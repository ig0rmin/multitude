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

#ifndef RADIANT_PLATFORM_HPP
#define RADIANT_PLATFORM_HPP

// C++0x check
#if __cplusplus > 199711L
  #define RADIANT_CPP0X 1
#endif

// Is this OS X?
#ifdef __APPLE_CC__
#   define RADIANT_OSX 1
// Is this Windows?
#elif WIN32
#	include <yvals.h>
#   define RADIANT_WIN32 1
// Check for TR1
#	ifndef _HAS_TR1
#		error "Compiler TR1 support was not found. Please install Visual Studio 2008 Service Pack 1 or use a newer compiler."
#	endif
// Is this Linux?
#elif __linux__
#   define RADIANT_LINUX 1
#else
#   error "Unsupported platform!"
#endif

#endif
