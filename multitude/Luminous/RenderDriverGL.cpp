#include "Luminous/RenderDriverGL.hpp"
#include "Luminous/VertexAttributeBinding.hpp"
#include "Luminous/VertexDescription.hpp"
#include "Luminous/HardwareBuffer.hpp"
#include "Luminous/ShaderConstantBlock.hpp"
#include "Luminous/ShaderProgram.hpp"
#include "Luminous/GLUtils.hpp"

#include <Nimble/Matrix4.hpp>
#include <Radiant/RefPtr.hpp>

#include <cassert>
#include <map>
#include <vector>

namespace Luminous
{
  class RenderDriverGL::Impl
  {
  public:
    // Some types
    struct SizedHandle
    {
      SizedHandle() : handle(0), version(0), size(0) {}
      GLuint handle;
      uint64_t version;
      size_t size;
    };
    struct ResourceHandle
    {
      ResourceHandle() : handle(0), version(0) {}
      GLuint handle;
      uint64_t version;
    };
    typedef SizedHandle BufferHandle;
    typedef ResourceHandle BindingHandle;
    typedef ResourceHandle ShaderHandle;

    typedef std::map<RenderResource::Id, BufferHandle> BufferList;
    typedef std::map<RenderResource::Id, BindingHandle> BindingList;
    typedef std::map<RenderResource::Id, ShaderHandle> ShaderList;

    // Current state of a single thread
    struct ThreadState
    {
      ThreadState()
        : currentProgram(0)
      {
      }

      GLuint currentProgram;    // Currently bound shader program
      BufferList bufferList;    // Active buffer objects
      BindingList bindings;     // Active buffer bindings
      ShaderList shaders;       // Active shaders
    };

  public:
    Impl(unsigned int threadCount)
      : resourceId(0)
      , m_threadResources(threadCount)
    {}

    RenderResource::Id createId()
    {
      return resourceId++;
    }

    //////////////////////////////////////////////////////////////////////////
    // @todo All these getOrCreate* functions have a lot of duplication. Should make better functions?
    BufferHandle getOrCreateBuffer(unsigned int threadIndex, RenderResource::Id id)
    {
      // See if we can find it in cache
      BufferList & res = m_threadResources[threadIndex].bufferList;
      BufferList::iterator it = res.find(id);
      if (it != res.end())
        return it->second;

      // No hit, create new buffer
      BufferHandle bufferHandle;
      glGenBuffers(1, &bufferHandle.handle);
      return bufferHandle;
    }

    BindingHandle getOrCreateBinding(unsigned int threadIndex, RenderResource::Id id)
    {
      BindingList & res = m_threadResources[threadIndex].bindings;
      BindingList::iterator it = res.find(id);
      if (it != res.end())
        return it->second;
      return BindingHandle();
    }

    ShaderHandle getOrCreateShaderProgram(unsigned int threadIndex, RenderResource::Id id)
    {
      ShaderList & res = m_threadResources[threadIndex].shaders;
      ShaderList::iterator it = res.find(id);
      if (it != res.end())
        return it->second;
      return ShaderHandle();
    }

    ShaderHandle getOrCreateShader(unsigned int threadIndex, RenderResource::Id id, ShaderType type)
    {
      ShaderList & res = m_threadResources[threadIndex].shaders;
      ShaderList::iterator it = res.find(id);
      if (it != res.end())
        return it->second;

      ShaderHandle shaderHandle;
      shaderHandle.handle = glCreateShader(GLUtils::getShaderType(type));
      return shaderHandle;
    }

    void updateBuffer(unsigned int threadIndex, RenderResource::Id id, BufferHandle handle)
    {
      m_threadResources[threadIndex].bufferList[id] = handle;
    }
    void updateBinding(unsigned int threadIndex, RenderResource::Id id, BindingHandle handle)
    {
      m_threadResources[threadIndex].bindings[id] = handle;
    }

    void updateShader(unsigned int threadIndex, RenderResource::Id id, ShaderHandle handle)
    {
      m_threadResources[threadIndex].shaders[id] = handle;
    }

  public:
    RenderResource::Id resourceId;              // Next free available resource ID
    std::vector<ThreadState> m_threadResources;   // Thread resources
  };

  RenderDriverGL::RenderDriverGL(unsigned int threadCount)
    : m_impl(new RenderDriverGL::Impl(threadCount))
  {

  }

  RenderDriverGL::~RenderDriverGL()
  {
    delete m_impl;
  }

  std::shared_ptr<VertexDescription> RenderDriverGL::createVertexDescription()
  {
    return std::make_shared<VertexDescription>();
  }

  std::shared_ptr<VertexAttributeBinding> RenderDriverGL::createVertexAttributeBinding()
  {
    return std::make_shared<VertexAttributeBinding>(m_impl->createId());
  }

  std::shared_ptr<HardwareBuffer> RenderDriverGL::createVertexBuffer()
  {
    return std::make_shared<HardwareBuffer>(m_impl->createId(), BT_VertexBuffer);
  }

  std::shared_ptr<ShaderConstantBlock> RenderDriverGL::createConstantBuffer()
  {
    return std::make_shared<ShaderConstantBlock>(m_impl->createId());
  }

  std::shared_ptr<ShaderProgram> RenderDriverGL::createShaderProgram()
  {
    return std::make_shared<ShaderProgram>(m_impl->createId());
  }

  std::shared_ptr<ShaderGLSL> RenderDriverGL::createShader(ShaderType type)
  {
    return std::make_shared<ShaderGLSL>(m_impl->createId(), type);
  }

  void RenderDriverGL::preFrame(unsigned int threadIndex)
  {

  }

  void RenderDriverGL::postFrame(unsigned int threadIndex)
  {

  }

  void RenderDriverGL::bind(unsigned int threadIndex, const ShaderProgram & program)
  {
    Impl::ShaderHandle programHandle = m_impl->getOrCreateShaderProgram(threadIndex, program.resourceId());
    
    // Recreate if the program is dirty
    if (programHandle.version < program.version()) {
      glDeleteProgram(programHandle.handle);
      programHandle.handle = glCreateProgram();
      programHandle.version = program.version();
      m_impl->updateShader(threadIndex, program.resourceId(), programHandle);
    }

    // Check if any of the shaders are dirty
    bool needsRelink = false;
    for (size_t i = 0; i < program.shaderCount(); ++i) {
      const std::shared_ptr<ShaderGLSL> & shader = program.shader(i);
      Impl::ShaderHandle shaderHandle = m_impl->getOrCreateShader(threadIndex, shader->resourceId(), shader->type());
      if (shaderHandle.version < shader->version()) {
        needsRelink = true;
        // Detach old (if exists), recompile and relink
        glDetachShader(programHandle.handle, shaderHandle.handle);
        const GLchar * text = shader->text().toAscii().data();
        const GLint length = shader->text().length();
        glShaderSource(shaderHandle.handle, 1, &text, &length);
        glCompileShader(shaderHandle.handle);
        GLint compiled = GL_FALSE;
        glGetShaderiv(shaderHandle.handle, GL_COMPILE_STATUS, &compiled);
        if (compiled == GL_TRUE) {
          glAttachShader(programHandle.handle, shaderHandle.handle);
          m_impl->updateShader(threadIndex, shader->resourceId(), shaderHandle);
        }
        else {
          /// @todo Dump error log if failed to compile
          Radiant::error("Failed to compile shader");
        }
      }
    }
    if (needsRelink) {
      /// @todo Check for linking errors
      glLinkProgram(programHandle.handle);
    }

    glUseProgram(programHandle.handle);
  }

  void RenderDriverGL::bind(unsigned int threadIndex, const HardwareBuffer & buffer)
  {
    Impl::BufferHandle bufferHandle = m_impl->getOrCreateBuffer(threadIndex, buffer.resourceId());

    // Bind buffer
    GLenum bufferTarget = Luminous::GLUtils::getBufferType(buffer.type());
    glBindBuffer(bufferTarget, bufferHandle.handle);

    // Update if dirty
    if (bufferHandle.version < buffer.version()) {
      // Update or reallocate the resource
      if (buffer.size() != bufferHandle.size)
        glBufferData(bufferTarget, buffer.size(), buffer.data(), buffer.usage() );
      else 
        glBufferSubData(bufferTarget, 0, buffer.size(), buffer.data());

      // Update cache handle
      bufferHandle.version = buffer.version();
      bufferHandle.size = buffer.size();
      m_impl->updateBuffer(threadIndex, buffer.resourceId(), bufferHandle);
    }
  }

  void RenderDriverGL::bind(unsigned int threadIndex, const VertexAttributeBinding & binding)
  {
    Impl::BindingHandle bindingHandle = m_impl->getOrCreateBinding(threadIndex, binding.resourceId());

    if (bindingHandle.version < binding.version()) {
      // Recreate the binding
      /// @todo I don't imagine these can really be updated, right?
      glDeleteVertexArrays(1, &bindingHandle.handle);
      glGenVertexArrays(1, &bindingHandle.handle);
      glBindVertexArray(bindingHandle.handle);

      // Bind all vertex buffers
      for (size_t i = 0; i < binding.bindingCount(); ++i) {
        VertexAttributeBinding::Binding b = binding.binding(i);
        // Attach buffer
        bind(threadIndex, *b.buffer);
        // Set buffer attributes
        for (size_t attrIndex = 0; attrIndex < b.description->attributeCount(); ++attrIndex) {
          VertexAttribute attr = b.description->attribute(attrIndex);
          /// @todo Should cache these locations somewhere
          GLint location = glGetAttribLocation(m_impl->m_threadResources[threadIndex].currentProgram, attr.name.toAscii().data());
          GLenum normalized = (attr.normalized ? GL_TRUE : GL_FALSE);
          glVertexAttribPointer(location, attr.count, GLUtils::getDataType(attr.type), normalized, b.description->vertexSize(), (const GLvoid*)attr.offset);
        }

        // Update cache
        bindingHandle.version = binding.version();
        m_impl->updateBinding(threadIndex, binding.resourceId(), bindingHandle);
      }
    }
    else
      glBindVertexArray(bindingHandle.handle);
  }

  void RenderDriverGL::bind(unsigned int threadIndex, const ShaderConstantBlock & constants)
  {
    Impl::BufferHandle bufferHandle = m_impl->getOrCreateBuffer(threadIndex, constants.resourceId());

    // Bind buffer
    glBindBuffer(GL_UNIFORM_BUFFER_EXT, bufferHandle.handle);

    /// @todo: duplicate code with HardwareBuffer binding: split up more

    // Update if dirty
    if (bufferHandle.version < constants.version()) {
      // Update or reallocate the resource
      if (constants.size() != bufferHandle.size)
        /// @todo What should be the usage type for a uniform buffer? Should it be flexible like a normal buffer?
        glBufferData(GL_UNIFORM_BUFFER_EXT, constants.size(), constants.data(), GL_DYNAMIC_DRAW);
      else 
        glBufferSubData(GL_UNIFORM_BUFFER_EXT, 0, constants.size(), constants.data());

      // Update cache handle
      bufferHandle.version = constants.version();
      bufferHandle.size = constants.size();
      m_impl->updateBuffer(threadIndex, constants.resourceId(), bufferHandle);
    }
  }

  void RenderDriverGL::unbind(unsigned int, const HardwareBuffer & buffer)
  {
    /// @todo Should we verify it's a valid resource?
    GLenum bufferTarget = Luminous::GLUtils::getBufferType(buffer.type());
    glBindBuffer(bufferTarget, 0);
  }

  void RenderDriverGL::unbind(unsigned int, const VertexAttributeBinding &)
  {
    /// @todo Should we verify it's a valid resource?
    glBindVertexArray(0);
  }

  void RenderDriverGL::unbind(unsigned int, const ShaderConstantBlock &)
  {
    glBindBuffer(GL_UNIFORM_BUFFER_EXT, 0);
  }

  void RenderDriverGL::unbind(unsigned int, const ShaderProgram &)
  {
    glUseProgram(0);
  }

  void RenderDriverGL::clear(ClearMask mask, const Radiant::Color & color, double depth, int stencil)
  {
    /// @todo check current target for depth and stencil buffers?

    GLbitfield glMask = 0;
    // Clear color buffer
    if (mask & CM_Color) {
      glClearColor(color.red(), color.green(), color.blue(), color.alpha());
      glMask |= GL_COLOR_BUFFER_BIT;
    }
    // Clear depth buffer
    if (mask & CM_Depth) {
      glClearDepth(depth);
      glMask |= GL_DEPTH_BUFFER_BIT;
    }
    // Clear stencil buffer
    if (mask & CM_Stencil) {
      glClearStencil(stencil);
      glMask |= GL_DEPTH_BUFFER_BIT;
    }
    glClear(glMask);
  }

  void RenderDriverGL::draw(PrimitiveType type, size_t offset, size_t primitives)
  {
    GLenum mode = GLUtils::getPrimitiveType(type);
    glDrawArrays(mode, (GLint) offset, (GLsizei) primitives);
  }

  void RenderDriverGL::drawIndexed(PrimitiveType type, size_t offset, size_t primitives)
  {
    /// @todo allow other index types (unsigned byte, unsigned short and unsigned int)
    GLenum mode = GLUtils::getPrimitiveType(type);
    glDrawElements(mode, (GLsizei) primitives, GL_UNSIGNED_INT, (const GLvoid *)((sizeof(uint) * offset)));
  }
}
