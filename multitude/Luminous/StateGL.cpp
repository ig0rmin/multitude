#include "StateGL.hpp"
#include "RenderDriverGL.hpp"

namespace Luminous
{
  StateGL::StateGL(unsigned int threadIndex, RenderDriverGL & driver)
    : m_threadIndex(threadIndex)
    , m_driver(driver)
  {
  }

  void StateGL::addTask(std::function<void ()> task)
  {
    m_driver.addTask(std::move(task));
  }

  void StateGL::initGl()
  {
    m_opengl = &m_driver.opengl();
    m_opengl45 = m_driver.opengl45();
  }
}
