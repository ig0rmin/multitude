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

#ifndef LUMINOUS_GLRESOURCE_HPP
#define LUMINOUS_GLRESOURCE_HPP

#include <Luminous/Export.hpp>
#include <Radiant/MemCheck.hpp>

#include <cstddef>

namespace Luminous
{
  class GLResources;

  /// Abstract base class for OpenGL resource objects
  /** This class is used to represent arbitrary OpenGL resources per
      OpenGL context.

      GLResource is used for two purposes:

      1) It used to store OpenGL resource handles (texture- FBO-, VBO-
      identifiers etc) so that they can be deleted after when the
      object that needs them is to be deleted. This feature is
      necessary in threaded rendering, where one thread is used to run
      application logic and other threads do the actual rendering.


      2) Keep track of OpenGL resource usage. Individual resource
      object can notify the host system about their OpenGL memory
      usage, which is useful for avoiding out-of-memory
      situations. For example textures can be thrown out of the GPU
      memory more aggressively if the system is running out of GPU
      memory.

      A typical way to use this class is that you derive classes from
      this one. The derived class can include more than one OpenGL
      resource, depending on how you need to use them.



      <PRE>

      MyWidget is the object that uses OpenGL resources

      class MyWidget : public Widget
      {
      public:

        MyResource is used so that these resources can be freed after
        the actual MyWidget object has been deleted.

        class MyResource : GLResource
    {
    public:
      bool load();

          The three resources below are used for tracking OpenGL
          memory usage.

      Texture2D m_tex1;
      Texture2D m_tex2;
      GPUTextureFont m_font;
    };


    MyWidget()
    {}

    virtual void update()
    {
      Needs to access style sheet
      Needs to access font layout information
    }

    virtual void render(GLResources * resources)
    {
      This function probably needs to access style sheet

      MyResource * rs = dynamic_cast<MyResource *>
        (resources->getResource(this));
      if(!rs) {
        rs = new MyResource;
        rs->load();

            These three are needed for GPU memory tracking

        rs->m_tex1.setResources(resources);
        rs->m_tex2.setResources(resources);
        rs->m_font.setResources(resources);
        resources->addResource(this, rs);
      }

      Do stuff using "rs"
    }

      protected:

    CPUTextureFont m_font;
      };
     </PRE>
  */
  class LUMINOUS_API GLResource
#ifdef MULTI_MEMCHECK
    : public Radiant::MemCheck
#endif
  {
    enum { PERSISTENT = -2 };

  public:
    friend class GLResources;

    /// Constructs a new resource and associates it with the given resources
    /// collection
    GLResource(GLResources * resources = 0);
    virtual ~GLResource();

    /// Returns the resources collection this resource belongs to
    GLResources * resources() { return m_resources; }

    /** Change the current resource host. This function can only be
      called once.
    @param resources new resource host*/
    virtual void setResources(GLResources * resources);

    /// Returns the number of bytes this object consumes at the moment
    virtual long consumesBytes();

    /// Sets the generation for the resource. Used to determine if a resource is up-to-date for a rendering context.
    void setGeneration(size_t g) { m_generation = g; }
    /// Returns the generation of the resource.
    size_t generation() const { return m_generation; }

    /// Tells if this object is persistent
    /** Persistent GPU resources should not be deleted, unless the Collectable is deleted.
        Default implementation returns false. */
    bool persistent() { return m_deleteOnFrame == PERSISTENT; }

    /// Makes this resource persistent
    /** Generally #GLResource objects are deleted if they are not used for a given number of frames.
        By making the objects persistent this can be avoided.

        @param b If true this #GLResource will not be deleted based on the timeout parameters.
    */
    void setPersistent(bool b);

  protected:

    /// To be called when changing memory consumption
    void changeByteConsumption(long deallocated, long allocated);

  private:
    GLResources * m_resources;

    long m_deleteOnFrame;
    size_t m_generation;
  };
}

#endif
