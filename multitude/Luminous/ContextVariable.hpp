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

#ifndef LUMINOUS_CONTEXTVARIABLE_HPP
#define LUMINOUS_CONTEXTVARIABLE_HPP

#include "Collectable.hpp"
#include "GLResources.hpp"

namespace Luminous {

  class GLResources;

  /// Template class for accessing per-context graphics resources
  /** The purpose of this class is to simplify the management of OpenGL resources,
      for threaded applications. */
  template <class T>
  class ContextVariableT : public Luminous::Collectable
  {
  public:
    ContextVariableT() {}
    virtual ~ContextVariableT() {}
    /// Gets a reference to the OpenGL resource
    /** Before calling this function you should have a valid OpenGL context, with
        the right GLResources main object set for this thread.

        Since this function
        gets a direct pointer to the GLResources object, it is slightly faster than
        the function without this argument.

        @param wasCreated true if the resource was created, false if it already existed
        @return Returns a reference to the OpenGL resource.
    */
    inline T & ref(bool * wasCreated = 0)
    {
      if(wasCreated)
        *wasCreated = false;

      Luminous::GLResources * grs = Luminous::GLResources::getThreadResources();
      T * obj = dynamic_cast<T *> (grs->getResource(this));
      if(!obj) {
        obj = new T(grs);
        grs->addResource(this, obj);

        if(wasCreated)
          *wasCreated = true;
      }

      return *obj;
    }

    /** @copydoc ref
        @param rs Pointer to the OpenGL resource container
        @param wasCreated true if the resource was created, false if it already existed
        */
    inline T & ref(GLResources * rs, bool * wasCreated = 0)
    {
      if(!rs) {
        return ref(wasCreated);
      }

      if(wasCreated)
        *wasCreated = false;

      T * obj = dynamic_cast<T *> (rs->getResource(this));
      if(!obj) {
        obj = new T(rs);
        rs->addResource(this, obj);

        if(wasCreated)
          *wasCreated = true;
      }

      return *obj;
    }

  };

} // namespace

#endif // CONTEXTVARIABLE_HPP
