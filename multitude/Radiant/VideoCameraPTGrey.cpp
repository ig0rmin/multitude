/* Copyright (C) 2007-2013: Multi Touch Oy, Helsinki University of Technology
 * and others.
 *
 * This file is licensed under GNU Lesser General Public License (LGPL),
 * version 2.1. The LGPL conditions can be found in file "LGPL.txt" that is
 * distributed with this source package or obtained from the GNU organization
 * (www.gnu.org).
 * 
 */

#include "Platform.hpp"

#ifdef CAMERA_DRIVER_PGR

#include "VideoCameraPTGrey.hpp"

#include "Mutex.hpp"
#include "Sleep.hpp"
#include "Radiant.hpp"
#include "Trace.hpp"

#include <map>
#ifndef WIN32
#include <flycapture/FlyCapture2.h>
#else
#include <FlyCapture2.h>
#endif

#define NUM_BUFFERS 10

namespace Radiant
{
  /* It seems that ptgrey drivers are not 100% thread-safe. To overcome this we
     use a mutex to lock captureImage calls to one-thread at a time. */
  static Mutex s_cameraMutex;

  typedef std::map<uint64_t, FlyCapture2::PGRGuid> GuidMap;
  GuidMap g_guidMap;

  static FlyCapture2::FrameRate framerateToPGR(Radiant::FrameRate fr)
  {
    switch(fr) {
    case FPS_5:
      return FlyCapture2::FRAMERATE_3_75;
      break;
    case FPS_10:
      return FlyCapture2::FRAMERATE_7_5;
      break;
    case FPS_30:
      return FlyCapture2::FRAMERATE_30;
      break;
    case FPS_60:
      return FlyCapture2::FRAMERATE_60;
      break;
    case FPS_120:
      return FlyCapture2::FRAMERATE_120;
    default:
    case FPS_15:
      return FlyCapture2::FRAMERATE_15;
      break;
    }
  }

  static std::map<FlyCapture2::PropertyType, VideoCamera::FeatureType> g_propertyFC2ToRadiant;
  static std::map<VideoCamera::FeatureType, FlyCapture2::PropertyType> g_propertyRadiantToFC2;

  static VideoCamera::FeatureType propertyToRadiant(FlyCapture2::PropertyType id)
  {
    static bool once = true;
    if(once) {
      g_propertyFC2ToRadiant.insert(std::make_pair(FlyCapture2::BRIGHTNESS, VideoCamera::BRIGHTNESS));
      g_propertyFC2ToRadiant.insert(std::make_pair(FlyCapture2::AUTO_EXPOSURE, VideoCamera::EXPOSURE));
      g_propertyFC2ToRadiant.insert(std::make_pair(FlyCapture2::SHARPNESS, VideoCamera::SHARPNESS));
      g_propertyFC2ToRadiant.insert(std::make_pair(FlyCapture2::WHITE_BALANCE, VideoCamera::WHITE_BALANCE));
      g_propertyFC2ToRadiant.insert(std::make_pair(FlyCapture2::HUE, VideoCamera::HUE));
      g_propertyFC2ToRadiant.insert(std::make_pair(FlyCapture2::SATURATION, VideoCamera::SATURATION));
      g_propertyFC2ToRadiant.insert(std::make_pair(FlyCapture2::GAMMA, VideoCamera::GAMMA));
      g_propertyFC2ToRadiant.insert(std::make_pair(FlyCapture2::IRIS, VideoCamera::IRIS));
      g_propertyFC2ToRadiant.insert(std::make_pair(FlyCapture2::FOCUS, VideoCamera::FOCUS));
      g_propertyFC2ToRadiant.insert(std::make_pair(FlyCapture2::ZOOM, VideoCamera::ZOOM));
      g_propertyFC2ToRadiant.insert(std::make_pair(FlyCapture2::PAN, VideoCamera::PAN));
      g_propertyFC2ToRadiant.insert(std::make_pair(FlyCapture2::TILT, VideoCamera::TILT));
      g_propertyFC2ToRadiant.insert(std::make_pair(FlyCapture2::SHUTTER, VideoCamera::SHUTTER));
      g_propertyFC2ToRadiant.insert(std::make_pair(FlyCapture2::GAIN, VideoCamera::GAIN));
      g_propertyFC2ToRadiant.insert(std::make_pair(FlyCapture2::TRIGGER_MODE, VideoCamera::TRIGGER));
      g_propertyFC2ToRadiant.insert(std::make_pair(FlyCapture2::TRIGGER_DELAY, VideoCamera::TRIGGER_DELAY));
      g_propertyFC2ToRadiant.insert(std::make_pair(FlyCapture2::FRAME_RATE, VideoCamera::FRAME_RATE));
      g_propertyFC2ToRadiant.insert(std::make_pair(FlyCapture2::TEMPERATURE, VideoCamera::TEMPERATURE));

      once = false;
    }

    assert(g_propertyFC2ToRadiant.find(id) != g_propertyFC2ToRadiant.end());

    return g_propertyFC2ToRadiant[id];
  }

  static FlyCapture2::PropertyType propertyToFC2(VideoCamera::FeatureType id)
  {
    static bool once = true;
    if(once) {
      g_propertyRadiantToFC2.insert(std::make_pair(VideoCamera::BRIGHTNESS, FlyCapture2::BRIGHTNESS));
      g_propertyRadiantToFC2.insert(std::make_pair(VideoCamera::EXPOSURE, FlyCapture2::AUTO_EXPOSURE));
      g_propertyRadiantToFC2.insert(std::make_pair(VideoCamera::SHARPNESS, FlyCapture2::SHARPNESS));
      g_propertyRadiantToFC2.insert(std::make_pair(VideoCamera::WHITE_BALANCE, FlyCapture2::WHITE_BALANCE));
      g_propertyRadiantToFC2.insert(std::make_pair(VideoCamera::HUE, FlyCapture2::HUE));
      g_propertyRadiantToFC2.insert(std::make_pair(VideoCamera::SATURATION, FlyCapture2::SATURATION));
      g_propertyRadiantToFC2.insert(std::make_pair(VideoCamera::GAMMA, FlyCapture2::GAMMA));
      g_propertyRadiantToFC2.insert(std::make_pair(VideoCamera::IRIS, FlyCapture2::IRIS));
      g_propertyRadiantToFC2.insert(std::make_pair(VideoCamera::FOCUS, FlyCapture2::FOCUS));
      g_propertyRadiantToFC2.insert(std::make_pair(VideoCamera::ZOOM, FlyCapture2::ZOOM));
      g_propertyRadiantToFC2.insert(std::make_pair(VideoCamera::PAN, FlyCapture2::PAN));
      g_propertyRadiantToFC2.insert(std::make_pair(VideoCamera::TILT, FlyCapture2::TILT));
      g_propertyRadiantToFC2.insert(std::make_pair(VideoCamera::SHUTTER, FlyCapture2::SHUTTER));
      g_propertyRadiantToFC2.insert(std::make_pair(VideoCamera::GAIN, FlyCapture2::GAIN));
      g_propertyRadiantToFC2.insert(std::make_pair(VideoCamera::TRIGGER, FlyCapture2::TRIGGER_MODE));
      g_propertyRadiantToFC2.insert(std::make_pair(VideoCamera::TRIGGER_DELAY, FlyCapture2::TRIGGER_DELAY));
      g_propertyRadiantToFC2.insert(std::make_pair(VideoCamera::FRAME_RATE, FlyCapture2::FRAME_RATE));
      g_propertyRadiantToFC2.insert(std::make_pair(VideoCamera::TEMPERATURE, FlyCapture2::TEMPERATURE));

      once = false;
    }

    assert(g_propertyRadiantToFC2.find(id) != g_propertyRadiantToFC2.end());

    return g_propertyRadiantToFC2[id];
  }

  ////////////////////////////////////////////////////////////////////////////////
  ////////////////////////////////////////////////////////////////////////////////

  static FlyCapture2::BusManager * g_bus = 0;

  bool VideoCameraPTGrey::m_fakeFormat7 = true;

  void g_busResetCallback(void * /*param*/)
  {
    Radiant::info("FIREWIRE BUS RESET");
  }

  VideoCameraPTGrey::VideoCameraPTGrey(CameraDriver * driver)
    : VideoCamera(driver),
    m_state(UNINITIALIZED)
  {
  }

  VideoCameraPTGrey::~VideoCameraPTGrey()
  {
    m_image.freeMemory();
    if(m_state != UNINITIALIZED)
      close();
  }

  bool VideoCameraPTGrey::open(uint64_t euid, int , int , ImageFormat , FrameRate framerate)
  {
    m_fakeFormat7 = false;

    debugRadiant("VideoCameraPTGrey::open # %llx", (long long) euid);

    FlyCapture2::PGRGuid guid;

    // If the euid is zero, take the first camera
    if(euid == 0) {
      if(g_guidMap.empty()) {
        error("VideoCameraPTGrey::open # No Cameras found");
        return false;
      }
      guid = g_guidMap.begin()->second;
    } else {
      GuidMap::iterator it = g_guidMap.find(euid);
      if(it == g_guidMap.end()) {
        Radiant::error("VideoCameraPTGrey::open # guid not found");
        return false;
      }

      guid = it->second;
    }

    m_image.allocateMemory(IMAGE_GRAYSCALE, 640, 480);

    // Set BUFFER_FRAMES & capture timeout
    FlyCapture2::FC2Config config;
    config.grabMode = FlyCapture2::BUFFER_FRAMES;
    config.numBuffers = NUM_BUFFERS;
    config.bandwidthAllocation = FlyCapture2::BANDWIDTH_ALLOCATION_ON;
    config.isochBusSpeed = FlyCapture2::BUSSPEED_S400;
    config.asyncBusSpeed = FlyCapture2::BUSSPEED_ANY;
    config.grabTimeout = 0;
    config.numImageNotifications = 1;

    /// @todo Do we need this?
    // Guard g(s_cameraMutex);
    // Connect camera
    FlyCapture2::Error err = m_camera.Connect(&guid);
    if(err != FlyCapture2::PGRERROR_OK) {
      Radiant::error("VideoCameraPTGrey::open # %s", err.GetDescription());
      return false;
    }

    // Set video mode and framerate
    err = m_camera.SetVideoModeAndFrameRate(FlyCapture2::VIDEOMODE_640x480Y8, framerateToPGR(framerate));
    if(err != FlyCapture2::PGRERROR_OK) {
      Radiant::error("VideoCameraPTGrey::open # %s", err.GetDescription());
      return false;
    }

    err = m_camera.SetConfiguration(&config);
    if(err != FlyCapture2::PGRERROR_OK) {
      Radiant::error("VideoCameraPTGrey::open # %s", err.GetDescription());
      err.PrintErrorTrace();
      return false;
    }

    FlyCapture2::VideoMode vm;
    FlyCapture2::FrameRate fr;

    err = m_camera.GetVideoModeAndFrameRate(&vm, &fr);
    if(err != FlyCapture2::PGRERROR_OK) {
      Radiant::error("VideoCameraPTGrey::open # %s", err.GetDescription());
      err.PrintErrorTrace();
      return false;
    }

    // Set trigger delay to zero just in case
    FlyCapture2::TriggerDelay td;
    td.type = FlyCapture2::TRIGGER_DELAY;
    td.valueA = 0;
    td.valueB = 0;

    err = m_camera.SetTriggerDelay(&td, true);
    if(err != FlyCapture2::PGRERROR_OK) {
      Radiant::error("VideoCameraPTGrey::open # %s", err.GetDescription());
      err.PrintErrorTrace();
      return false;
    }

    m_state = OPENED;


    FlyCapture2::CameraInfo camInfo;
    err = m_camera.GetCameraInfo(&camInfo);
    if(err != FlyCapture2::PGRERROR_OK) {
      Radiant::error("VideoCameraPTGrey::open # %s", err.GetDescription());
    }

    m_info.m_vendor = camInfo.vendorName;
    m_info.m_model = camInfo.modelName;
    m_info.m_euid64 = euid;
    m_info.m_driver = driver()->driverName();

    return true;
  }

  bool VideoCameraPTGrey::openFormat7(uint64_t euid, Nimble::Recti roi, float fps, int mode)
  {
    fps = 180.0f;

    m_format7Rect = roi;
    if(m_fakeFormat7) {
      // Expand the ROI to include everything, this will be clamped later on.
      roi.set(0, 0, 100000, 100000);
    }

    debugRadiant("VideoCameraPTGrey::openFormat7 # %llx", (long long) euid);

    // Look up PGRGuid from our map (updated in queryCameras())
    GuidMap::iterator it = g_guidMap.find(euid);
    if(it == g_guidMap.end()) {
      Radiant::error("VideoCameraPTGrey::open # guid not found");
      return false;
    }

    FlyCapture2::PGRGuid guid = it->second;

    /// @todo Do we need this?
    // Guard g(s_cameraMutex);

    // Connect camera
    FlyCapture2::Error err = m_camera.Connect(&guid);
    if(err != FlyCapture2::PGRERROR_OK) {
      Radiant::error("VideoCameraPTGrey::openFormat7 # Connect %s", err.GetDescription());
      return false;
    }

    // Set BUFFER_FRAMES & capture timeout
    FlyCapture2::FC2Config config;
    err = m_camera.GetConfiguration(&config);
    if(err != FlyCapture2::PGRERROR_OK) {
      Radiant::error("VideoCameraPTGrey::openFormat7 # GetConfiguration %s", err.GetDescription());
      return false;
    }

    config.grabMode = FlyCapture2::BUFFER_FRAMES;
    config.numBuffers = NUM_BUFFERS;
    config.bandwidthAllocation = FlyCapture2::BANDWIDTH_ALLOCATION_ON;
    config.isochBusSpeed = FlyCapture2::BUSSPEED_S400;

    err = m_camera.SetConfiguration(&config);
    if(err != FlyCapture2::PGRERROR_OK) {
      Radiant::error("VideoCameraPTGrey::openFormat7 # SetConfiguration %s", err.GetDescription());
      return false;
    }

    // Make sure the image size is divisible by four
    int roiWidth = (roi.width() + 3) & ~0x3;
    int roiHeight = (roi.height() + 3) & ~0x3;
    roi.high().x += roiWidth - roi.width();
    roi.high().y += roiHeight - roi.height();

    // Query format7 info for the requested mode
    FlyCapture2::Format7Info f7info;
    f7info.mode = FlyCapture2::Mode(mode);

    bool supported;
    err = m_camera.GetFormat7Info(&f7info, &supported);
    if(err != FlyCapture2::PGRERROR_OK) {
      Radiant::error("VideoCameraPTGrey::openFormat7 # GetFormat7Info %s", err.GetDescription());
      return false;
    }

    // Set Format7 frame size
    FlyCapture2::Format7ImageSettings f7s;
    memset(&f7s, 0, sizeof(f7s));
    f7s.offsetX = roi.low().x;
    f7s.offsetY = roi.low().y;
    Nimble::Vector2i avail(f7info.maxWidth, f7info.maxHeight);
    avail -= roi.low();

    f7s.width = std::min(roi.width(), avail.x);
    f7s.height = std::min(roi.height(), avail.y);
    f7s.pixelFormat = FlyCapture2::PIXEL_FORMAT_MONO8;
    f7s.mode = FlyCapture2::Mode(mode);

    // Define set fps by adjusting the packet size

    // Cycles in the FireWire bus
    const int BUS_CYCLES_PER_SECOND = 8000;
    // How many bus cycles per frame we need?
    unsigned int busCyclesPerFrame = ceil(BUS_CYCLES_PER_SECOND / fps);
    // Frame size in bytes
    unsigned int frameSizeInBytes = f7s.width * f7s.height;
    // Needed packet size
    unsigned int packetSize = frameSizeInBytes / busCyclesPerFrame;

    // If the requested packetSize exceeds the maximum supported, clamp it
    if(packetSize > f7info.maxPacketSize) {
      Radiant::info("VideoCameraPTGrey::openFormat7 # requested camera fps (%f) is too high. Using slower.", fps);
      packetSize = f7info.maxPacketSize;
    }

    // Validate
    Radiant::info("Validating format7 settings...");
    FlyCapture2::Format7PacketInfo f7pi;
    memset(&f7pi, 0, sizeof(f7pi));
    err = m_camera.ValidateFormat7Settings(&f7s, &supported, &f7pi);
    if(err != FlyCapture2::PGRERROR_OK) {
      Radiant::error("VideoCameraPTGrey::openFormat7 # ValidateFormat7Settings %s", err.GetDescription());
      err.PrintErrorTrace();
    }

    Radiant::info("FORMAT7 SETTINGS:");
    Radiant::info("\tOffset %d %d", f7s.offsetX, f7s.offsetY);
    Radiant::info("\tSize %d %d [%d %d]", f7s.width, f7s.height,
                  f7info.maxWidth, f7info.maxHeight);
    Radiant::info("\tMode %d", f7s.mode);
    Radiant::info("\tPacket size: %d [%d, %d]", packetSize,
                  f7info.minPacketSize, f7info.maxPacketSize);

    Radiant::info("PACKET INFO");
    Radiant::info("\tRecommended packet size: %d", f7pi.recommendedBytesPerPacket);
    Radiant::info("\tMax bytes packet size: %d", f7pi.maxBytesPerPacket);
    Radiant::info("\tUnit bytes per packet: %d", f7pi.unitBytesPerPacket);

    // f7pi.recommendedBytesPerPacket = 1056;

    // err = m_camera.SetFormat7Configuration( &f7s, f7pi.recommendedBytesPerPacket);
    err = m_camera.SetFormat7Configuration( &f7s, f7pi.recommendedBytesPerPacket);

    roi.high().make(f7s.offsetX + f7s.width, f7s.offsetY + f7s.height);
    m_format7Rect.high() = roi.clamp(m_format7Rect.high());
    m_format7Rect.low() = roi.clamp(m_format7Rect.low());

    if(err != FlyCapture2::PGRERROR_OK) {
      Radiant::error("VideoCameraPTGrey::openFormat7 # SetFormat7Configuration %s",
                     err.GetDescription());
      err.PrintErrorTrace();
      return false;
    }

    // Allocate space for the image
    m_image.allocateMemory(IMAGE_GRAYSCALE, m_format7Rect.width(), m_format7Rect.height());

    m_state = OPENED;

    FlyCapture2::CameraInfo camInfo;
    err = m_camera.GetCameraInfo(&camInfo);
    if(err != FlyCapture2::PGRERROR_OK) {
      Radiant::error("VideoCameraPTGrey::openFormat7 # GetCameraInfo failed %s",
                     err.GetDescription());
    }

    m_info.m_vendor = camInfo.vendorName;
    m_info.m_model  = camInfo.modelName;
    m_info.m_euid64 = euid;
    m_info.m_driver = driver()->driverName();

    Radiant::info("VideoCameraPTGrey::openFormat7 # Success (%llx)", euid);

    return true;
  }

  bool VideoCameraPTGrey::start()
  {
    if(m_state != OPENED) {
      /* If the device is already running, then return true. */
      if(m_state == RUNNING)
        return true;

      error("VideoCameraPTGrey::start # State != OPENED (%llx)",
            (long long) m_info.m_euid64);
      return false;
    }

    /// @todo Do we need this?
    // Guard g(s_cameraMutex);
    FlyCapture2::Error err = m_camera.StartCapture();
    if(err != FlyCapture2::PGRERROR_OK) {
      Radiant::error("VideoCameraPTGrey::start # %s", err.GetDescription());
      err.PrintErrorTrace();
      return false;
    }

    m_state = RUNNING;

    return true;
  }

  bool VideoCameraPTGrey::stop()
  {
    if(m_state != RUNNING) {
      debugRadiant("VideoCameraPTGrey::stop # State != RUNNING");
      /* If the device is already stopped, then return true. */
      return m_state == OPENED;
    }

    Radiant::info("VideoCameraPTGrey::stop");

    /// @todo Do we need this?
    // Guard g(s_cameraMutex);
    FlyCapture2::Error err = m_camera.StopCapture();
    if(err != FlyCapture2::PGRERROR_OK) {
      Radiant::error("VideoCameraPTGrey::stop # %s", err.GetDescription());
      return false;
    }

    m_state = OPENED;

    return true;
  }

  bool VideoCameraPTGrey::close()
  {
    if(m_state == UNINITIALIZED)
      return true;

    Radiant::info("VideoCameraPTGrey::close");
    m_state = UNINITIALIZED;

    /// @todo Do we need this?
    // Guard g(s_cameraMutex);
    m_camera.Disconnect();

    return true;
  }

  uint64_t VideoCameraPTGrey::uid()
  {
    return m_info.m_euid64;
  }

  const Radiant::VideoImage * VideoCameraPTGrey::captureImage()
  {
    FlyCapture2::Image img;

    // Lock the global mutex so we don't retrieve multiple buffers at the same time
    {
      Guard g(s_cameraMutex);
      FlyCapture2::Error err = m_camera.RetrieveBuffer(&img);
      if(err != FlyCapture2::PGRERROR_OK) {
        Radiant::error("VideoCameraPTGrey::captureImage # %llx %s",
          m_info.m_euid64, err.GetDescription());
        return false;
      }
    }
    /*
    if(m_image.size() != img.GetDataSize()) {
      Radiant::info("ALLOCATED %dx%d bytes %d", m_image.width(), m_image.height(), m_image.size());
      Radiant::info("FRAME %dx%d stride %d bytes %d", img.GetCols(), img.GetRows(), img.GetStride(), img.GetDataSize());

      //assert(m_image.size() == img.GetDataSize());
    }
*/

    if(m_fakeFormat7 && m_format7Rect.width() > 1) {
      // info("FAKE FORMAT 7 CAPTURE");
      // Copy only part of the image
      /* The fake format7 mode. This is done so that one can use Format7 ROI even
         when the feature is broken. One many Windows systems this only causes BSODs.
      */
      int w = m_format7Rect.width();
      const unsigned char * src = img.GetData();
      src += img.GetCols() * m_format7Rect.low().y + m_format7Rect.low().x;
      unsigned char * dest = m_image.m_planes[0].m_data;

      for(int y = m_format7Rect.low().y; y < m_format7Rect.high().y; y++) {
        memcpy(dest, src, w);
        src += img.GetCols();
        dest += w;
      }
    }
    else
      memcpy(m_image.m_planes[0].m_data, img.GetData(), m_image.size());

    return &m_image;
  }

  VideoCamera::CameraInfo VideoCameraPTGrey::cameraInfo()
  {
    return m_info;

  }

  int VideoCameraPTGrey::width() const
  {
    return m_image.m_width;
  }

  int VideoCameraPTGrey::height() const
  {
    return m_image.m_height;
  }

  float VideoCameraPTGrey::fps() const
  {
    return -1;
  }

  ImageFormat VideoCameraPTGrey::imageFormat() const
  {
    return Radiant::IMAGE_GRAYSCALE;
  }

  unsigned int VideoCameraPTGrey::size() const
  {
    return width() * height() * sizeof(uint8_t);
  }

  void VideoCameraPTGrey::setFeature(FeatureType id, float value)
  {
    /// @todo Do we need this?
    // Guard g(s_cameraMutex);

    // debugRadiant("VideoCameraPTGrey::setFeature # %d %f", id, value);

    // If less than zero, use automatic mode
    if(value < 0.f) {
      setFeatureRaw(id, -1);
      return;
    }

    FlyCapture2::PropertyInfo pinfo;
    pinfo.type = propertyToFC2(id);

    FlyCapture2::Error err = m_camera.GetPropertyInfo(&pinfo);
    if(err != FlyCapture2::PGRERROR_OK) {
      debugRadiant("VideoCameraPTGrey::setFeature # Failed: \"%s\"",
                     err.GetDescription());
      return;
    }

    int32_t intVal = pinfo.min + (value * (pinfo.max - pinfo.min));

    setFeatureRaw(id, intVal);
  }

  void VideoCameraPTGrey::setFeatureRaw(FeatureType id, int32_t value)
  {
    // debugRadiant("VideoCameraPTGrey::setFeatureRaw # %d %d", id, value);

    FlyCapture2::Property prop;
    prop.type = propertyToFC2(id);

    m_camera.GetProperty(&prop);
    /*
    Radiant::info("DEBUG: BEFORE ADJUSTMENT");
    Radiant::info("type %d", prop.type);
    Radiant::info("present %d", prop.present);
    Radiant::info("abs control %d", prop.absControl);
    Radiant::info("one push %d", prop.onePush);
    Radiant::info("on/off %d", prop.onOff);
    Radiant::info("autoManual %d", prop.autoManualMode);
    Radiant::info("value A %d", prop.valueA);
    Radiant::info("value B %d", prop.valueB);
    Radiant::info("abs value %f", prop.absValue);
*/

    prop.valueA = value;
    prop.valueB = value;

    // Automatic or manual mode?
    prop.autoManualMode = value < 0 ? true : false;

    FlyCapture2::Error err = m_camera.SetProperty(&prop);
    if(err != FlyCapture2::PGRERROR_OK) {
      debugRadiant("VideoCameraPTGrey::setFeatureRaw # Failed: \"%s\"",
                     err.GetDescription());
      err.PrintErrorTrace();
    }
    /*
    m_camera.GetProperty(&prop);
    Radiant::info("DEBUG: AFTER ADJUSTMENT");
    Radiant::info("abs control %d", prop.absControl);
    Radiant::info("value A %d", prop.valueA);
    Radiant::info("value B %d", prop.valueB);
*/
  }

  bool VideoCameraPTGrey::enableTrigger(TriggerSource src)
  {
    FlyCapture2::TriggerMode tm;

    FlyCapture2::Error err = m_camera.GetTriggerMode(&tm);
    if(err != FlyCapture2::PGRERROR_OK) {
      Radiant::error("VideoCameraPTGrey::enableTrigger # %s", err.GetDescription());
      return false;
    }

    tm.onOff = true;
    tm.source = (unsigned int)(src);

    err = m_camera.SetTriggerMode(&tm);
    if(err != FlyCapture2::PGRERROR_OK) {
      Radiant::error("VideoCameraPTGrey::enableTrigger # %s", err.GetDescription());
      return false;
    }

    return true;
  }

  bool VideoCameraPTGrey::setTriggerMode(TriggerMode mode)
  {
    FlyCapture2::TriggerMode tm;

    FlyCapture2::Error err = m_camera.GetTriggerMode(&tm);
    if(err != FlyCapture2::PGRERROR_OK) {
      Radiant::error("VideoCameraPTGrey::enableTrigger # %s", err.GetDescription());
      return false;
    }

    tm.mode = (unsigned int)(mode);

    err = m_camera.SetTriggerMode(&tm);
    if(err != FlyCapture2::PGRERROR_OK) {
      Radiant::error("VideoCameraPTGrey::enableTrigger # %s", err.GetDescription());
      return false;
    }

    return true;
  }

  bool VideoCameraPTGrey::setTriggerPolarity(TriggerPolarity polarity)
  {
    FlyCapture2::TriggerMode tm;

    FlyCapture2::Error err = m_camera.GetTriggerMode(&tm);
    if(err != FlyCapture2::PGRERROR_OK) {
      Radiant::error("VideoCameraPTGrey::setTriggerPolarity # %s", err.GetDescription());
      return false;
    }

    tm.polarity = (unsigned int)(polarity);

    err = m_camera.SetTriggerMode(&tm);
    if(err != FlyCapture2::PGRERROR_OK) {
      Radiant::error("VideoCameraPTGrey::setTriggerPolarity # %s", err.GetDescription());
      return false;
    }

    return true;
  }

  bool VideoCameraPTGrey::disableTrigger()
  {
    // The TriggerMode is initialized to disabled state
    FlyCapture2::TriggerMode mode;

    FlyCapture2::Error err = m_camera.SetTriggerMode(&mode);
    if(err != FlyCapture2::PGRERROR_OK) {
      Radiant::error("VideoCameraPTGrey::disableTrigger # %s", err.GetDescription());
      return false;
    }

    return true;
  }

  void VideoCameraPTGrey::sendSoftwareTrigger()
  {
    FlyCapture2::Error err = m_camera.FireSoftwareTrigger();
    if(err != FlyCapture2::PGRERROR_OK) {
      Radiant::error("VideoCameraPTGrey::sendSoftwareTrigger # %s", err.GetDescription());
    }
  }

  bool VideoCameraPTGrey::setCaptureTimeout(int ms)
  {
    // Read the current camera configuration
    FlyCapture2::FC2Config config;
    FlyCapture2::Error err = m_camera.GetConfiguration(&config);
    if(err != FlyCapture2::PGRERROR_OK) {
      Radiant::error("VideoCameraPTGrey::setCaptureTimeout # %s", err.GetDescription());
      return false;
    }

    // Modify the capture timeout
    config.grabTimeout = ms;

    // Set the new configuration
    err = m_camera.SetConfiguration(&config);
    if(err != FlyCapture2::PGRERROR_OK) {
      Radiant::error("VideoCameraPTGrey::setCaptureTimeout # %s", err.GetDescription());
      return false;
    }

    return true;
  }

  void VideoCameraPTGrey::queryFeature(FlyCapture2::PropertyType id, std::vector<CameraFeature> * features)
  {
    FlyCapture2::PropertyInfo pinfo;
    pinfo.type = id;

    FlyCapture2::Error err = m_camera.GetPropertyInfo(&pinfo);
    if(err != FlyCapture2::PGRERROR_OK) {
      //Radiant::error("VideoCameraPTGrey::getFeatures # %s", err.GetDescription());
      return;
    }

    if(!pinfo.present) { debugRadiant("Skipping feature %d, not present", id); return; }

    VideoCamera::CameraFeature feat;
    feat.id = propertyToRadiant(id);

    feat.absolute_capable = pinfo.absValSupported;
    feat.abs_max = pinfo.absMax;
    feat.abs_min = pinfo.absMin;
    feat.available = pinfo.present;
    feat.max = pinfo.max;
    feat.min = pinfo.min;
    feat.on_off_capable = pinfo.onOffSupported;

    // Figure out supported modes
    feat.num_modes = 0;
    if(pinfo.manualSupported)
      feat.modes[feat.num_modes++] = MODE_MANUAL;

    if(pinfo.autoSupported)
      feat.modes[feat.num_modes++] = MODE_AUTO;

    if(pinfo.onePushSupported)
      feat.modes[feat.num_modes++] = MODE_ONE_PUSH_AUTO;

    FlyCapture2::Property prop;
    prop.type = pinfo.type;
    err = m_camera.GetProperty(&prop);
    if(err != FlyCapture2::PGRERROR_OK) {
      Radiant::error("VideoCameraPTGrey::getFeatures # %s", err.GetDescription());
      return;
    }

    feat.abs_value = prop.absValue;
    feat.value = prop.valueA;
    feat.is_on = prop.onOff;

    features->push_back(feat);
  }

  void VideoCameraPTGrey::getFeatures(std::vector<CameraFeature> * features)
  {
    features->clear();

    for(int type = FlyCapture2::BRIGHTNESS; type <= FlyCapture2::TEMPERATURE; type++)
      queryFeature(FlyCapture2::PropertyType(type), features);
  }

  //////////////////////////////////////////////////////////////////////
  //////////////////////////////////////////////////////////////////////
  static std::vector<VideoCamera::CameraInfo> g_cameras;

  CameraDriverPTGrey::CameraDriverPTGrey()
  {
  }

  size_t CameraDriverPTGrey::queryCameras(std::vector<VideoCamera::CameraInfo> & suppliedCameras)
  {
    static bool wasRun = false;
    if(wasRun) {
      suppliedCameras.insert(suppliedCameras.begin(), g_cameras.begin(), g_cameras.end());
      return g_cameras.size();
    }

    std::vector<VideoCamera::CameraInfo> myCameras;

    // Clear guid map
    g_guidMap.clear();

    if(!g_bus) g_bus = new FlyCapture2::BusManager();

    // g_bus->RegisterCallback(g_busResetCallback, FlyCapture2::BUS_RESET, 0, 0);

    // Get the number of available cameras
    unsigned int numCameras;
    FlyCapture2::Error err = g_bus->GetNumOfCameras(&numCameras);
    if(err != FlyCapture2::PGRERROR_OK) {
      Radiant::error("VideoCameraPTGrey::queryCameras # %s", err.GetDescription());
      return false;
    }

    for(uint32_t i = 0; i < numCameras; i++) {
      FlyCapture2::PGRGuid guid;

      // Query camera guid
      err = g_bus->GetCameraFromIndex(i, &guid);
      if(err != FlyCapture2::PGRERROR_OK) {
        Radiant::error("VideoCameraPTGrey::queryCameras # %s", err.GetDescription());
        return false;
      }

      // Connect camera based on guid
      FlyCapture2::Camera camera;
      err = camera.Connect(&guid);
      if(err != FlyCapture2::PGRERROR_OK) {
        Radiant::error("VideoCameraPTGrey::queryCameras # %s", err.GetDescription());
        return false;
      }

      // Query camera info
      FlyCapture2::CameraInfo cameraInfo;
      err = camera.GetCameraInfo(&cameraInfo);
      if(err != FlyCapture2::PGRERROR_OK) {
        Radiant::error("VideoCameraPTGrey::queryCameras # %s", err.GetDescription());
        return false;
      }

      uint64_t a = cameraInfo.configROM.nodeVendorId;
      uint64_t b = cameraInfo.configROM.chipIdHi;
      //uint64_t c = cameraInfo.configROM.unitSpecId;
      uint64_t d = cameraInfo.configROM.chipIdLo;

      // This is how they do it in libdc1394 (in enumeration.c)
      uint64_t uuid = (a << 40) | (b << 32) | (d);

      g_guidMap.insert(std::make_pair(uuid, guid));

      VideoCamera::CameraInfo myInfo;

      myInfo.m_vendor = cameraInfo.vendorName;
      myInfo.m_model = cameraInfo.modelName;
      myInfo.m_euid64 = uuid;
      myInfo.m_driver = driverName();

      myCameras.push_back(myInfo);
    }

    // Cache the results for later
    /// @todo caching the results is not so good idea, because you can't hotplug cameras now
    g_cameras = myCameras;
    // Append to the camera vector
    suppliedCameras.insert(suppliedCameras.end(), myCameras.begin(), myCameras.end());

    wasRun = true;

    return numCameras;
  }

  VideoCamera * CameraDriverPTGrey::createCamera()
  {
    return new VideoCameraPTGrey(this);
  }

}

#endif // CAMERA_DRIVER_PGR
