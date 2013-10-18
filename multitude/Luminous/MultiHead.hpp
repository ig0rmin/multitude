/* Copyright (C) 2007-2013: Multi Touch Oy, Helsinki University of Technology
 * and others.
 *
 * This file is licensed under GNU Lesser General Public License (LGPL),
 * version 2.1. The LGPL conditions can be found in file "LGPL.txt" that is
 * distributed with this source package or obtained from the GNU organization
 * (www.gnu.org).
 *
 */


#ifndef LUMINOUS_MULTIHEAD_HPP
#define LUMINOUS_MULTIHEAD_HPP

#include "Export.hpp"
#include "Collectable.hpp"
#include "GLKeyStone.hpp"
#include "HardwareColorCorrection.hpp"
#include "ColorCorrection.hpp"
#include "RGBCube.hpp"

#include <Nimble/Rect.hpp>
#include <Nimble/Vector4.hpp>

#include <memory>

#include <Valuable/AttributeString.hpp>
#include <Valuable/AttributeFloat.hpp>
#include <Valuable/AttributeBool.hpp>

#include <vector>


namespace Luminous {

  using Nimble::Rect;
  using Nimble::Vector2f;
  using Nimble::Vector2i;
  using Nimble::Vector4f;

  class RenderContext;

  /// Class for managing information on multiple OpenGL vindows/viewports. This
  /// class stores information about the layout of multiple OpenGL windows and
  /// viewports. This information is used in systems that render OpenGL into
  /// several windows at once. MultiHead also includes keystone information, so
  /// that keystoning can be done by using skewed OpenGL transformation
  /// matrices. Multihead only stores the information of the configuration.
  /// Changing any of the values during runtime will have no effect unless the
  /// windows are restarted.
  class LUMINOUS_API MultiHead : public Valuable::Node
  {
  public:

    class Window;

    /// An OpenGL area. Areas are roughly equivalent to OpenGL viewports.
    /// Multiple areas can share the same OpenGL context, as one window can
    /// have may areas inside it.
    /// @todo rename to ViewPort?
    class LUMINOUS_API Area : public Valuable::Node, public Collectable
    {
      MEMCHECKED_USING(Collectable);
    public:

      /// Constructs a new area for the given window
      Area();
      virtual ~Area();
      /// Deserializes this area from an archive element
      bool deserialize(const Valuable::ArchiveElement & element);

      /// Sets the geometry (size & offset) of the area
      /// @param x x offset
      /// @param y y offset
      /// @param w width
      /// @param h height
      /// @param copyToGraphics if true, the settings are copied to graphics coordinates
      void setGeometry(int x, int y, int w, int h, bool copyToGraphics = true);

      /// Sets the graphics geometry of the area
      /// @param x x coordinate of the graphics area
      /// @param y y coordinate of the graphics area
      /// @param w width of the graphics area
      /// @param h height of the graphics area
      void setGraphicsGeometry(int x, int y, int w, int h);

      void setSeams(Nimble::Vector4f seams);

      /// Area seams are used by projector setups to define regions that are
      /// edge-blended together.
      Nimble::Vector4f seams() const;

      /// Returns the width of the largest seam
      float maxSeam() const;

      /// Returns the keystone correction object
      /// @return keystone correction
      GLKeyStone & keyStone();
      /// @copydoc keyStone
      const GLKeyStone & keyStone() const;

      /// Location of the area origin in pixels in window coordinates
      const Vector2i & location() const;
      void setLocation(Nimble::Vector2i loc);
      /// Size of the area inside the window in pixels
      Nimble::Size size() const;
      void setSize(Nimble::Size size);

      /// Origin of the graphics coordinates in the area
      const Nimble::Vector2f graphicsLocation(bool withseams = true) const;
      void setGraphicsLocation(Nimble::Vector2f l);

      /// The size of the graphics inside this area (virtual pixels)
      const Nimble::SizeF graphicsSize(bool withseams = true) const;
      void setGraphicsSize(Nimble::SizeF size);

      /// The bounds of the graphics. In principle this method only combines
      /// the information you get with graphicsLocation() and graphicsSize().
      /// In practice however that information is not exactly correct as the
      /// bounds also need to include the small extra areas, if edge-blending
      /// is used.
      /// @return graphics bounds rectangle
      const Rect & graphicsBounds() const;

      /// Convert window coordinates to graphics coordinates.
      /// @param loc location in window coordinates
      /// @param windowheight height of the window
      /// @param[out] insideArea set to true if the location is inside this
      /// area; otherwise false
      /// @return vector in graphics coordinates.
      Nimble::Vector2f windowToGraphics(Nimble::Vector2f loc, int windowheight, bool & insideArea) const;

      /// Convert graphics coordinates to window coordinates
      /// @param loc location in graphics coordinates
      /// @param windowheight height of the window
      /// @param[out] insideArea set to true if the location is inside this
      /// area; otherwise false
      /// @return vector in graphics coordinates.
      Nimble::Vector2f graphicsToWindow(Nimble::Vector2f loc, int windowheight, bool & insideArea) const;

      /// Get the view transformation (projection) matrix defined by the area
      /// @return view transformation defined by the area
      Nimble::Matrix4 viewTransform() const;

      /// Swaps the width and height of the graphics size
      void swapGraphicsWidthHeight();

      /// Get the 3D color-correction RGB cube associated with this area. The
      /// 3D color-correction is used by Cornerstone.
      /// @return 3D color-correction
      RGBCube & rgbCube();
      /// @copydoc rgbCube
      const RGBCube & rgbCube() const;

      /// Get the 2D color-correction associated with this area. The 2D
      /// color-correction is used by the hardware on the cells.
      /// @return 2D color-correction
      ColorCorrection & colorCorrection();
      /// @copydoc colorCorrection
      const ColorCorrection & colorCorrection() const;

      /// Get the viewport defined by the area in window coordinates.
      /// @return the viewport defined by the area
      Nimble::Recti viewport() const;

      /// Element type used for serialization
      /// @return "area"
      QByteArray type() const OVERRIDE { return "area"; }

      bool readElement(const Valuable::ArchiveElement & element) OVERRIDE;

      /// How the area is rendered to the framebuffer
      enum RenderMethod {
        /// Render to framebuffer, the read-back to texture, then re-render applying keystone-correction.
        METHOD_TEXTURE_READBACK,
        /// Render to framebuffer using keystone-correction
        METHOD_MATRIX_TRICK
      };

    private:

      void updateBBox();

      GLKeyStone m_keyStone;
      Valuable::AttributeVector2i   m_location;
      Valuable::AttributeVector2i   m_size;
      Valuable::AttributeVector2f   m_graphicsLocation;
      Valuable::AttributeVector2f   m_graphicsSize;
      Valuable::AttributeVector4f   m_seams;
      Valuable::AttributeInt        m_method;
      Rect m_graphicsBounds;

      Collectable m_colorCorrectionTextureKey;
      ColorCorrection m_colorCorrection;
      RGBCube m_rgbCube;
    };

    /** One OpenGL window.
    A window is responsible for one OpenGL context.*/
    class Window : public Valuable::Node
    {
    public:
      friend class Area;

      /// Constructs a new window for the given screen
      LUMINOUS_API Window(MultiHead * screen = 0);
      LUMINOUS_API ~Window();

      /// Set the location and size of this window
      void setGeometry(int x, int y, int w, int h)
      {
        m_location = Nimble::Vector2i(x, y);
        m_size = Nimble::Vector2i(w, h);
      }

      /// Resize the window, and automatically one child area
      /** This method is used when the window contains only one child
          area, and automatially changes the size of the area to match
          the area of the window.
      @param size new window size */
      LUMINOUS_API void resizeEvent(Nimble::Size size);

      /// Number of areas that this window holds
      size_t areaCount() const { return m_areas.size(); }
      /// Get one of the areas
      Area & area(size_t i) { return * m_areas[i].get(); }
      /// Get one of the areas
      const Area & area(size_t i) const { return * m_areas[i].get(); }

      /// Returns the union of the areas' graphics bounds
      LUMINOUS_API Nimble::Rect graphicsBounds() const;

      /// Sets the seam for each area
      LUMINOUS_API void setSeam(float seam);

      /// Adds an area to the window
      LUMINOUS_API void addArea(std::unique_ptr<Area> a);

      /// Location of the window in desktop coordinates
      const Vector2i & location() const { return m_location; }
      void setLocation(Nimble::Vector2i loc) { m_location = loc; }

      /// Size of the window on the computer display
      Nimble::Size size() const { return Nimble::Size(*m_size); }
      void setSize(Nimble::Size size) { m_size = size.toVector(); }

      /// Returns the width of this window in pixels
      /// @return width of the window
      int width() const { return m_size.x(); }
      /// Returns the height of this window in pixels
      /// @return height of the window
      int height() const { return m_size.y(); }

      /** Convert a coordinate from screen to graphics coordinates.
          This class traverses through all the areas to find an area
          that would include given location. It is entirely possible
          none of the areas contains the given coordinate, as there
          can be gaps between areas (due to keystoning, for example).

          @param loc The location in screen coordinates.

          @param convOk set to true or false depending on whether the
          conversion could be carried out.

          @return the point in graphics coordinates
      */
      LUMINOUS_API Nimble::Vector2f windowToGraphics(Nimble::Vector2f loc, bool & convOk) const;
      /// @copydoc windowToGraphics
      LUMINOUS_API QPointF windowToGraphics(QPointF loc, bool & convOk) const;

      /// Should the window be frameless
      bool frameless() const { return m_frameless; }
      void setFrameless(bool frameless) { m_frameless = frameless; }

      /// Should the window be full-screen
      bool fullscreen() const { return m_fullscreen; }
      /// Sets the fullscreen flag
      void setFullscreen(bool f) { m_fullscreen = f; }
      /// Should the window be resizeable
      bool resizeable() const { return m_resizeable; }
      void setResizeable(bool resizeable) { m_resizeable = resizeable; }

      /// Screen number for the window. Use -1 for default screen.
      int screennumber() const { return m_screennumber; }
      void setScreennumber(int s) { m_screennumber = s; }

      /// Number of samples per pixel for full-screen anti-aliasing
      int antiAliasingSamples() const { return m_fsaaSamplesPerPixel; }
      void setAntiAliasingSamples(int samplesPerPixel) { m_fsaaSamplesPerPixel = samplesPerPixel; }

      /// Maximum upload limit for GPU texture and buffer uploads
      int64_t uploadLimit() const { return m_uploadLimit; }
      void setUploadLimit(int64_t limit) { m_uploadLimit = limit; }

      /// Minimum upload limit for GPU texture and buffer uploads
      int64_t uploadMargin() const { return m_uploadMargin; }
      void setUploadMargin(int64_t margin) { m_uploadMargin = margin; }

      /// Direct rendering mode enabled
      /// @return true if direct rendering is enabled, false is rendering
      /// is done through an off-screen render target
      bool directRendering() const { return m_directRendering; }
      /// Set direct rendering mode
      void setDirectRendering(bool enable) { m_directRendering = enable; }

      /// Return the screen configuration that this Window belongs to
      const MultiHead * screen() const { return m_screen; }

      /// Remove all areas for all windows.
      void deleteAreas();

      /// Checks if software color correction is in use for the specified area.
      /// This function returns true if 3D RGB cube color-correction is used,
      /// false if the 2D color-correction is used or the 3D color-correction is
      /// disabled.
      /// @param areaIndex area index
      /// @return true if 3D color-correction is used; otherwise false
      bool isAreaSoftwareColorCorrected(int areaIndex) const;

      /// Get the window rectangle
      /// @return window rectangle
      Nimble::Recti getRect() const;

      /// Element type used during serialization
      /// @return "window"
      QByteArray type() const { return "window"; }

    private:
      LUMINOUS_API virtual bool readElement(const Valuable::ArchiveElement & ce);

      MultiHead                *m_screen;
      Valuable::AttributeVector2i   m_location;
      Valuable::AttributeVector2i   m_size;
      Valuable::AttributeBool       m_frameless;
      Valuable::AttributeBool       m_fullscreen;
      Valuable::AttributeBool       m_resizeable;
      Valuable::AttributeInt        m_fsaaSamplesPerPixel;
      // GPU upload limits
      /// PCIe bandwidth
      /// PCIe 1.0 x16: 4GB/sec (2001)
      /// PCIe 2.0 x16: 8GB/sec (2007)
      /// PCIe 3.0 x16: 15.8GB/sec (2011)
      Valuable::AttributeInt64      m_uploadLimit;
      Valuable::AttributeInt64      m_uploadMargin;

      Valuable::AttributeBool       m_directRendering;
      Valuable::AttributeInt        m_screennumber; // for X11

      std::vector<std::unique_ptr<Area> > m_areas;
    };

    /// Construct an empty configuration
    MultiHead();
    /// Destructor
    virtual ~MultiHead();

    /// Get the number of areas
    /// @return number of areas
    size_t areaCount();

    /// Get an area. This method traverses all the windows to find the area
    /// with given index.
    /// @param index window index to look for
    /// @param[out] winptr pointer to a window if the area is found
    /// @return area with the given index or the first area if no match is found
    Area & area(size_t index, Window ** winptr = 0);

    /// Get the number of windows
    /// @return number of windows in the configuration
    size_t windowCount() const { return m_windows.size(); }
    /// Access the ith window
    /// @param i window index
    /// @return the window
    Window & window(size_t i);
    /// @copydoc window
    const Window & window(size_t i) const;

    /// Returns the total graphics size
    Rect graphicsBounds() const;

    /// Returns the size of the total display in graphics pixels
    Nimble::SizeF size()
    { return Nimble::SizeF(width(), height()); }

    /// Total width of the display area, in graphics pixels.
    int width();
    /// Total height of the display area, in graphics pixels.
    int height();

    /// Get the DPMS settings. This function returns a triplet of values
    /// corresponding to the DPMS settings. The values are in seconds for
    /// 'standby', 'suspend', and 'power off'.
    /// @return DPMS settings
    /// @sa setDpms
    Nimble::Vector3i dpms() const { return m_dpms; }
    /// Set the DPMS settings.
    /// @param dpms dpms settings
    /// @sa dpms
    void setDpms(const Nimble::Vector3i & dpms);

    bool deserialize(const Valuable::ArchiveElement & element);

    /// Adds a window to the collection
    void addWindow(std::unique_ptr<Window> w);

    /// Raise the edited flag for the configuration. Typically used to check if
    /// the settings need to be saved when exiting an application.
    /// @param edited true to set the flag; false to disable it
    void setEdited(bool edited) { m_edited = edited; }

    /// Check if the edited flag has been set. This function checks if the
    /// configuration has been modified.
    /// @return true if the configuration was changed; otherwise false
    bool isEdited() const { return m_edited; }

    /// Check if all windows should be iconified.
    /// @return true if the windows should be iconified; otherwise false
    bool iconify() const { return m_iconify; }
    /// Set the iconify flag for windows
    /// @param iconify true to iconify windows
    void setIconify(bool iconify) { m_iconify = iconify; }

    /// @cond

    const HardwareColorCorrection & hwColorCorrection() const
    {
      return m_hwColorCorrection;
    }

    HardwareColorCorrection & hwColorCorrection()
    {
      return m_hwColorCorrection;
    }

    /// @endcond

    /// Remove all windows from the configuration
    void deleteWindows();

    /// Get the dots-per-inch
    /// @return the DPI of the display
    float dpi() const;
    /// Set the DPI of the display
    /// @param dpi dots-per-inch of the display
    void setDpi(float dpi);

    virtual void eventProcess(const QByteArray & messageId, Radiant::BinaryData & data);

    /// Is the hardware color-correction enabled. Hardware color-correction is
    /// implemented with the video-multiplex hardware inside MultiTaction Cells.
    bool isHardwareColorCorrectionEnabled() const { return m_hwColorCorrectionEnabled; }
    void setHardwareColorCorrection(bool enabled) { m_hwColorCorrectionEnabled = enabled; }

    /// Create a default fullscreen configuration for a single 1080p display
    void createFullHDConfig();

  private:
    virtual bool readElement(const Valuable::ArchiveElement & ce);
    virtual void dpmsChanged();

    std::vector<std::unique_ptr<Window> > m_windows;
    Valuable::AttributeBool m_iconify;
    Valuable::AttributeVector3i m_dpms;
    Valuable::AttributeFloat m_dpi;
    Valuable::AttributeBool m_hwColorCorrectionEnabled;
    HardwareColorCorrection m_hwColorCorrection;

    bool m_edited;
  };

}

#endif
