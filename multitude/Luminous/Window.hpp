/* COPYRIGHT
 *
 * This file is part of Luminous.
 *
 * Copyright: MultiTouch Oy, Helsinki University of Technology and others, 2007-2013
 *
 * This file is licensed under GNU Lesser General Public
 * License (LGPL), version 2.1. The LGPL conditions can be found in 
 * file "LGPL.txt" that is distributed with this source package or obtained 
 * from the GNU organization (www.gnu.org).
 * 
 */

#ifndef WINDOW_HPP
#define WINDOW_HPP

#include "Export.hpp"
#include "WindowEventHook.hpp"

#include <Nimble/Vector2.hpp>

namespace Luminous
{

  /// Virtual base classes for OpenGL windows
  class LUMINOUS_API Window
  {
  public:
    /// Creates the base definitions for windows
    Window();
    virtual ~Window();

    /// Queries if the window is closed.
    /// This would happen if the user closes the window.
    /// @return true if the window has been closed
    bool isFinished() const;

    /// Sets the full-screen mode of the window
    void setFullscreen(bool fullscreen);

    /// Update window system (mouse & keyboard) events
    virtual void poll() = 0;
    /// Swap OpenGL buffers
    virtual void swapBuffers() = 0;

    /// Sets the OpenGL context for the current thread
    virtual void makeCurrent() = 0;

    /// Returns the width of the window
    int width() const;
    /// Returns the height of the window
    int height() const;

    void setWidth(int w) { m_width = w; }
    void setHeight(int h) { m_height = h; }

    /// Sets the object for sending window events
    virtual void setEventHook(WindowEventHook * hook);
    /// A pointer to the window event callback listener
    WindowEventHook * eventHook() const;

    virtual void init() {}

    /// Virtual function for cleaning up window resources
    virtual void deinit() {}

    virtual void minimize() = 0;
    virtual void maximize() = 0;
    virtual void restore() = 0;

    Nimble::Vector2i position() const { return m_pos; }
    void setPosition(Nimble::Vector2i pos) { m_pos = pos; }

    virtual void showCursor(bool visible) = 0;

  private:
    ///@cond
    bool m_finished;
    bool m_fullscreen;
    int m_width;
    int m_height;
    Nimble::Vector2i m_pos;

    WindowEventHook * m_eventHook;
    ///@endcond
  };

}

#endif
