/* COPYRIGHT
 *
 * This file is part of Radiant.
 *
 * Copyright: MultiTouch Oy, Helsinki University of Technology and others.
 *
 * See file "Radiant.hpp" for authors and more details.
 *
 * This file is licensed under GNU Lesser General Public
 * License (LGPL), version 2.1. The LGPL conditions can be found in 
 * file "LGPL.txt" that is distributed with this source package or obtained 
 * from the GNU organization (www.gnu.org).
 * 
 */

#ifndef RADIANT_VIDEO_CAMERA_HPP
#define RADIANT_VIDEO_CAMERA_HPP

#include <Radiant/Export.hpp>
#include <Radiant/VideoInput.hpp>

#include <Nimble/Rect.hpp>

#include <stdint.h>

#include <QString>

namespace Radiant {
  class CameraDriverFactory;
  class CameraDriver;

  /// VideoCamera provides a common interface for different video cameras. Each new camera driver should implement this interface and also implement the CameraDriver interface as well.
  class RADIANT_API VideoCamera : public VideoInput
  {
  public:
    /// Constructs a new video
    /// @param driver camera driver from where the camera was instantiated from
    VideoCamera(CameraDriver * driver);
    virtual ~VideoCamera();

    /// A container of basic camera information. CameraInfo objects
    /// are used to store information about a particular camera.
    class CameraInfo {
    public:
      CameraInfo()
        : m_euid64(0)
      {}

      /// The 64-bit unique FireWire identifier
      int64_t m_euid64;
      /// Vendor name, in a human-readable format
      QString m_vendor;
      /// Camera model, in a human-readable format
      QString m_model;
      /// Driver that was used for this camera
      QString m_driver;
    };

    /// Camera feature modes
    enum FeatureMode {
      MODE_MANUAL = 0,
      MODE_AUTO,
      MODE_ONE_PUSH_AUTO,
      MODE_MAX
    };

    /// Camera feature types
    enum FeatureType {
      BRIGHTNESS = 0,
      EXPOSURE,
      SHARPNESS,
      WHITE_BALANCE,
      HUE,
      SATURATION,
      GAMMA,
      SHUTTER,
      GAIN,
      IRIS,
      FOCUS,
      TEMPERATURE,
      TRIGGER,
      TRIGGER_DELAY,
      WHITE_SHADING,
      FRAME_RATE,
      ZOOM,
      PAN,
      TILT,
      OPTICAL_FILTER,
      CAPTURE_SIZE,
      CAPTURE_QUALITY,
      FEATURE_TYPE_MAX
    };

    /// Camera external trigger sources
    enum TriggerSource {
      TRIGGER_SOURCE_0 = 0,
      TRIGGER_SOURCE_1,
      TRIGGER_SOURCE_2,
      TRIGGER_SOURCE_3,
      TRIGGER_SOURCE_SOFTWARE,
      TRIGGER_SOURCE_MAX
    };

    /// Camera external trigger modes
    enum TriggerMode {
      TRIGGER_MODE_0 = 0,
      TRIGGER_MODE_1,
      TRIGGER_MODE_2,
      TRIGGER_MODE_3,
      TRIGGER_MODE_4,
      TRIGGER_MODE_5,
      TRIGGER_MODE_14 = 14,
      TRIGGER_MODE_15 = 15,
      TRIGGER_MODE_MAX
    };

    /// Camera external trigger polarity
    enum TriggerPolarity {
      TRIGGER_ACTIVE_LOW = 0,
      TRIGGER_ACTIVE_HIGH,
      TRIGGER_ACTIVE_UNDEFINED
    };

    /// A container of basic camera feature information.
    struct CameraFeature {
      /// Id number of the feature
      FeatureType id;
      /// Minimum value for the feature (integer)
      uint32_t min;
      /// Maximum value for the feature (integer)
      uint32_t max;
      /// Current value for the feature (integer)
      uint32_t value;
      /// Is this feature available?
      bool available;
      /// Does this feature support absolute mode?
      bool absolute_capable;
      /// Can this feature be read?
      bool readout_capable;
      /// Does this feature support on/off mode?
      bool on_off_capable;
      /// Does this feature support polarity
      bool polarity_capable;
      /// Is this feature enabled
      bool is_on;

      /// Current mode the feature is in
      FeatureMode current_mode;
      /// Number of different modes this feature supports
      uint32_t num_modes;
      /// The different modes this feature supports
      FeatureMode modes[MODE_MAX];

      /// Current value for the feature (float)
      float abs_value;
      /// Minimum value for the feature (float)
      float abs_min;
      /// Maximum value for the feature (float)
      float abs_max;
    };

    /** Opens a connection to the camera and initializes the image capture parameters.
  @param euid hardware id of the camera
  @param width width of the camera image
  @param height height of the camera image
  @param fmt image format
  @param framerate framerate of the camera
  */
    virtual bool open(uint64_t euid, int width, int height, ImageFormat fmt = IMAGE_UNKNOWN, FrameRate framerate = FPS_IGNORE) = 0;
    /** Opens a connection to the camera and sets up format7 image capture.
      @param cameraeuid hardware id of the camera
      @param roi region of interest
      @param fps desired frames per second
      @param mode desired format7 mode
      */
    virtual bool openFormat7(uint64_t cameraeuid, Nimble::Recti roi, float fps, int mode) = 0;

    /** Gets the different features that the camera supports */ virtual void
    getFeatures(std::vector<CameraFeature> * features) = 0;

    /** Sets the relative value of a feature based on the minimum and maximum
    values. If the value is negative, the feature is set to automatic mode.
    @param id id of the feature to set
    @param value value to set
      */
    virtual void setFeature(FeatureType id, float value) = 0;
    /** Sets the absolute value of a feature.
      @param id id of the feature to set
      @param value value of the feature to set
    */
    virtual void setFeatureRaw(FeatureType id, int32_t value) = 0;
    /** Returns a human-readable name for a feature given the feature id */
    static const char * featureName(FeatureType id);

    /** Checks if the given camera feature supports a certain mode */
    static bool hasMode(const CameraFeature & feature, FeatureMode mode);
    /** Checks if the given camera feature supports a automatic mode */
    static bool hasAutoMode(const CameraFeature & feature)
    { return hasMode(feature, MODE_AUTO); }
    /** Checks if the given camera feature supports a manual mode */
    static bool hasManualMode(const CameraFeature & feature)
    { return hasMode(feature, MODE_MANUAL); }

    /** Sets the value of the PAN feature */
    virtual void setPan(float value);
    /** Sets the value of the TILT feature */
    virtual void setTilt(float value);
    /** Sets the value of the GAMMA feature */
    virtual void setGamma(float value);
    /** Sets the value of the SHUTTER feature */
    virtual void setShutter(float value);
    /** Sets the value of the GAIN feature */
    virtual void setGain(float value);
    /** Sets the value of the EXPOSURE feature */
    virtual void setExposure(float value);
    /** Sets the value of the BRIGHTNESS feature */
    virtual void setBrightness(float value);
    /** Sets the value of the FOCUS feature */
    virtual void setFocus(float value);
    /** Sets the timeout for waiting for a new frame from the camera */
    virtual bool setCaptureTimeout(int ms) = 0;
    /** @deprecated not implemented */
    virtual void setWhiteBalance(float u_to_blue, float v_to_red) = 0;

    /// Enables external triggering of the camera
    virtual bool enableTrigger(TriggerSource src) = 0;
    /// Sets the triggering mode for the camera
    virtual bool setTriggerMode(TriggerMode mode) = 0;
    /// Sets the trigger polarity of the camera
    virtual bool setTriggerPolarity(TriggerPolarity polarity) = 0;
    /// Disables external trigger for the camera
    virtual bool disableTrigger() = 0;
    /// Sends a software trigger signal to the camera
    virtual void sendSoftwareTrigger() = 0;

    /// Returns information about this particular camera object
    virtual CameraInfo cameraInfo() = 0;

    /** Returns the number of frames that would be immediately readable.
      Currently, this function is only implemented for the libdc1394 driver.
     */
    virtual int framesBehind() const = 0;

    /// Returns an instance of the camera driver factory
    static CameraDriverFactory & drivers();

    /// Returns the driver which created this camera
    CameraDriver * driver() { return m_driver; }
    private:
    CameraDriver * m_driver;
  };

}

#endif
