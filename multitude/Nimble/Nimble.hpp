/* Copyright (C) 2007-2013: Multi Touch Oy, Helsinki University of Technology
 * and others.
 *
 * This file is licensed under GNU Lesser General Public License (LGPL),
 * version 2.1. The LGPL conditions can be found in file "LGPL.txt" that is
 * distributed with this source package or obtained from the GNU organization
 * (www.gnu.org).
 * 
 */

#ifndef NIMBLE_NIMBLE_HPP
#define NIMBLE_NIMBLE_HPP

#include <Radiant/Trace.hpp>


#define debugNimble(...) (Radiant::trace("Nimble", Radiant::DEBUG, __VA_ARGS__))
/// Nimble library is a collection of C++ classes for 2D/3D graphics.

/** Nimble is used mostly for simple arithmetic/geometric
    calculations. The code is optimized for performance, thus there
    are a lot of inline functions. Also, the vector and matrix classes
    <b>do not</b> initialize any of their members.

    The matrix and vector classes use right-handed coordinates.

    \b Copyright: The Nimble library has been developed by Helsinki
    University of Technology, Telecommunications Software and
    Multimedia Laboratory in the Digital Interactive Virtual Acoustics
    (DIVA, 1999-2005, as part of the Fluid interaction framework). It
    has been developed further by Helsinki Institute for Information
    Technology (HIIT, 2006-2007) and MultiTouch Oy (2007-2008).
    
    Nimble is released under the GNU Lesser General Public License
    (LGPL), version 2.1.

    @author Tommi Ilmonen, Janne Kontkanen, Esa Nuuros
    
*/

namespace Nimble {
  template <typename T> class Vector2T;
  template <typename T> class Vector3T;
  template <typename T> class Vector4T;
  template <typename T> class Matrix2T;
  template <typename T> class Matrix3T;
  template <typename T> class Matrix4T;
}

#endif
