/* COPYRIGHT
 *
 * This file is part of Valuable.
 *
 * Copyright: MultiTouch Oy, Helsinki University of Technology and others, 2007-2013
 *
 * This file is licensed under GNU Lesser General Public
 * License (LGPL), version 2.1. The LGPL conditions can be found in 
 * file "LGPL.txt" that is distributed with this source package or obtained 
 * from the GNU organization (www.gnu.org).
 * 
 */

#ifndef VALUABLE_EXPORT_HPP
#define VALUABLE_EXPORT_HPP

#include "Radiant/Platform.hpp"

// Import by default
#ifdef VALUABLE_EXPORT
#define VALUABLE_API MULTI_DLLEXPORT
#else
#define VALUABLE_API MULTI_DLLIMPORT
#endif

// Needed by std::istream & operator>>(std::istream & is, StyleValue & value);
#ifndef STYLISH_API
#ifdef STYLISH_EXPORT
#define STYLISH_API MULTI_DLLEXPORT
#else
#define STYLISH_API MULTI_DLLIMPORT
#endif
#endif

#endif
