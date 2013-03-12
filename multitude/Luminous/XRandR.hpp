/* Copyright (C) 2007-2013: Multi Touch Oy, Helsinki University of Technology
 * and others.
 *
 * This file is licensed under GNU Lesser General Public License (LGPL),
 * version 2.1. The LGPL conditions can be found in file "LGPL.txt" that is
 * distributed with this source package or obtained from the GNU organization
 * (www.gnu.org).
 * 
 */

#ifndef LUMINOUS_XRANDR_HPP
#define LUMINOUS_XRANDR_HPP

/// @cond

#include "Export.hpp"

#include <Nimble/Rect.hpp>

class QString;
namespace Luminous
{
  class LUMINOUS_API XRandR
  {
  public:
    XRandR();
    bool getGeometry(int screen, const QString & display, Nimble::Recti & rect);
  };
}

/// @endcond

#endif // LUMINOUS_XRANDR_HPP
