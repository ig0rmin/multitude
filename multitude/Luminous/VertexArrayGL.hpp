/* Copyright (C) 2007-2013: Multi Touch Oy, Helsinki University of Technology
 * and others.
 *
 * This file is licensed under GNU Lesser General Public License (LGPL),
 * version 2.1. The LGPL conditions can be found in file "LGPL.txt" that is
 * distributed with this source package or obtained from the GNU organization
 * (www.gnu.org).
 * 
 */

#ifndef LUMINOUS_VERTEXARRAYGL_HPP
#define LUMINOUS_VERTEXARRAYGL_HPP

#include "ResourceHandleGL.hpp"

#include <QRegion>

#include <set>
#include <memory>

namespace Luminous
{
  class VertexArrayGL : public ResourceHandleGL
  {
  public:
    LUMINOUS_API VertexArrayGL(StateGL & state);
    LUMINOUS_API ~VertexArrayGL();

    LUMINOUS_API VertexArrayGL(VertexArrayGL && t);
    LUMINOUS_API VertexArrayGL & operator=(VertexArrayGL && t);

    LUMINOUS_API void bind();

    LUMINOUS_API void upload(const VertexArray & vertexArray, ProgramGL * program);

    LUMINOUS_API int generation() const { return m_generation; }

  private:
    void setVertexAttributes(const VertexArray & vertexArray, ProgramGL * program);
    void setVertexDescription(const VertexDescription & description, ProgramGL * program);

    int m_generation;

    std::set<std::shared_ptr<BufferGL> > m_associatedBuffers;
  };
}

#endif
