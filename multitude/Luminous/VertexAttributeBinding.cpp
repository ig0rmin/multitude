#include "Luminous/VertexAttributeBinding.hpp"

#include <algorithm>

namespace Luminous
{
  class VertexAttributeBinding::Impl
  {
  public:
    typedef std::vector< VertexAttributeBinding::Binding > Bindings;
    Bindings bindings;
  };

  bool operator==(const VertexAttributeBinding::Binding & lhs, const std::shared_ptr<HardwareBuffer> & rhs) { return lhs.buffer == rhs; }

  VertexAttributeBinding::VertexAttributeBinding(RenderResource::Id id)
    : RenderResource(id, RT_VertexArray)
    , m_impl(new VertexAttributeBinding::Impl())
  {
  }

  VertexAttributeBinding::~VertexAttributeBinding()
  {
    delete m_impl;
  }

  void VertexAttributeBinding::addBinding(const std::shared_ptr<HardwareBuffer> & buffer, const std::shared_ptr<VertexDescription> & description)
  {
    // Add the binding if it doesn't already exist
    Impl::Bindings::const_iterator it = std::find(m_impl->bindings.begin(), m_impl->bindings.end(), buffer);
    if (it == m_impl->bindings.end()) {
      Binding binding;
      binding.buffer = buffer;
      binding.description = description;
      m_impl->bindings.push_back(binding);
      
      invalidate();
    }
  }

  void VertexAttributeBinding::removeBinding(const std::shared_ptr<HardwareBuffer> & buffer)
  {
    Impl::Bindings::iterator it = std::find(m_impl->bindings.begin(), m_impl->bindings.end(), buffer);
    if (it != m_impl->bindings.end()) {
      m_impl->bindings.erase( it );
      invalidate();
    }
  }

  void VertexAttributeBinding::clear()
  {
    if (!m_impl->bindings.empty()) {
      m_impl->bindings.clear();
      invalidate();
    }
  }

  size_t VertexAttributeBinding::bindingCount() const
  {
    return m_impl->bindings.size();
  }

  const VertexAttributeBinding::Binding & VertexAttributeBinding::binding(size_t index) const
  {
    assert( index < bindingCount() );
    return m_impl->bindings[index];
  }
}