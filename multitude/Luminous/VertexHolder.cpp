#include "VertexHolder.hpp"

#include "RenderContext.hpp"
#include "Utils.hpp"

namespace Luminous
{

  VertexHolder::VertexHolder()
  {
    strcpy(m_vertextype, "Undefined");
  }

  VertexHolder::~VertexHolder()
  {}

  RenderPacket::RenderPacket()
    : m_program(0),
      m_func(RectVertex::render)
  {}

  RenderPacket::~RenderPacket()
  {}

#define VERTEX_ATTRIB_STEP(prog, paramName, paramRef, objRef) \
  VertexAttribArrayStep step_##paramName (prog, #paramName, sizeof(paramRef) / 4, GL_FLOAT, GL_FALSE, sizeof(objRef), \
    offsetBytes(paramRef, objRef), func)

  void RectVertex::render(RenderContext &r, RenderPacket & rp)
  {
    if(! & rp)
      return;

    if(rp.empty())
      return;

    Utils::glCheck("RectVertex::render # 1");

    assert(rp.program() != 0);

    const int vsize = sizeof(RectVertex);

    RectVertex & vr = * rp.vertexData<RectVertex>();

    GLSLProgramObject & prog = * rp.program();

    prog.setUniformMatrix4("view_transform", r.viewTransform());

    Utils::glCheck("RectVertex::render # 2");

    rp.vbo().bind();
    rp.vbo().fill(rp.vertexData<RectVertex>(), rp.vertices().bytes(), Luminous::VertexBuffer::DYNAMIC_DRAW);

    const char * func = "RectVertex::render";

    VERTEX_ATTRIB_STEP(prog, location, vr.m_location, vr);
    VERTEX_ATTRIB_STEP(prog, color, vr.m_color, vr);
    VERTEX_ATTRIB_STEP(prog, tex_coord, vr.m_texCoord, vr);
    VERTEX_ATTRIB_STEP(prog, use_tex, vr.m_useTexture, vr);

    VERTEX_ATTRIB_STEP(prog, object_transform_r1, vr.m_objectTransform[0], vr);
    VERTEX_ATTRIB_STEP(prog, object_transform_r2, vr.m_objectTransform[1], vr);
    VERTEX_ATTRIB_STEP(prog, object_transform_r3, vr.m_objectTransform[2], vr);

    glDrawArrays(GL_TRIANGLE_STRIP, 0, GLsizei(rp.vertices().count<RectVertex>()));

    rp.clear();
    rp.vbo().unbind(); // Should not really need call this
    Utils::glCheck("RectVertex::render # 3");
  }

  void CircleVertex::render(RenderContext &r, RenderPacket & rp)
  {
    if(! & rp)
      return;

    if(rp.empty())
      return;

    Utils::glCheck("CircleVertex::render # 1");

    assert(rp.program() != 0);

    CircleVertex & vr = * rp.vertexData<CircleVertex>();

    GLSLProgramObject & prog = * rp.program();

    prog.setUniformMatrix4("view_transform", r.viewTransform());

    Utils::glCheck("CircleVertex::render # 2");

    rp.vbo().bind();
    rp.vbo().fill(rp.vertexData<CircleVertex>(), rp.vertices().bytes(), Luminous::VertexBuffer::DYNAMIC_DRAW);

    const char * func = "RectVertex::render"; // Needed by the macros below

    VERTEX_ATTRIB_STEP(prog, location, vr.m_location, vr);
    VERTEX_ATTRIB_STEP(prog, color, vr.m_color, vr);
    VERTEX_ATTRIB_STEP(prog, tex_coord, vr.m_texCoord, vr);
    VERTEX_ATTRIB_STEP(prog, obj_coord, vr.m_objCoord, vr);
    VERTEX_ATTRIB_STEP(prog, use_tex, vr.m_useTexture, vr);

    VERTEX_ATTRIB_STEP(prog, object_transform_r1, vr.m_objectTransform[0], vr);
    VERTEX_ATTRIB_STEP(prog, object_transform_r2, vr.m_objectTransform[1], vr);
    VERTEX_ATTRIB_STEP(prog, object_transform_r3, vr.m_objectTransform[2], vr);

    glDrawArrays(GL_TRIANGLE_STRIP, 0, GLsizei(rp.vertices().count<RectVertex>()));

    rp.clear();
    rp.vbo().unbind(); // Should not really call this
    Utils::glCheck("CircleVertex::render # 3");
  }

}
