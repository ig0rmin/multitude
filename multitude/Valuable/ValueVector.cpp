/* COPYRIGHT
 *
 * This file is part of Valuable.
 *
 * Copyright: MultiTouch Oy, Helsinki University of Technology and others.
 *
 * See file "Valuable.hpp" for authors and more details.
 *
 * This file is licensed under GNU Lesser General Public
 * License (LGPL), version 2.1. The LGPL conditions can be found in 
 * file "LGPL.txt" that is distributed with this source package or obtained 
 * from the GNU organization (www.gnu.org).
 * 
 */

// The purpose of this file is to include ValueVector.hpp and ValueVectorImpl.hpp in the
// Valuable build, thereby including any template instantiations defined in those files

#include "ValueVector.hpp"
#include "ValueVectorImpl.hpp"

namespace Valuable {

  template class ValueVector<Nimble::Vector2f>;
  template class ValueVector<Nimble::Vector3f>;
  template class ValueVector<Nimble::Vector4f>;
  
  template class ValueVector<Nimble::Vector2i>;
  template class ValueVector<Nimble::Vector3i>;
  template class ValueVector<Nimble::Vector4i>;
}

