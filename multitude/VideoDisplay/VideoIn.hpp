/* COPYRIGHT
 *
 * This file is part of VideoDisplay.
 *
 * Copyright: MultiTouch Oy, Helsinki University of Technology and others.
 *
 * See file "VideoDisplay.hpp" for authors and more details.
 *
 * This file is licensed under GNU Lesser General Public
 * License (LGPL), version 2.1. The LGPL conditions can be found in 
 * file "LGPL.txt" that is distributed with this source package or obtained 
 * from the GNU organization (www.gnu.org).
 * 
 */


#ifndef VIDEODISPLAY_VIDEO_IN_HPP
#define VIDEODISPLAY_VIDEO_IN_HPP

#include "Export.hpp"

#include <Nimble/Vector2.hpp>

#include <Radiant/Condition.hpp>
#include <Radiant/VideoImage.hpp>
#include <Radiant/IODefs.hpp>
#include <Radiant/Mutex.hpp>
#include <Radiant/RefPtr.hpp>
#include <Radiant/Thread.hpp>
#include <Radiant/TimeStamp.hpp>

#include <sys/types.h>

#include <vector>

// This library-internal class
/// @cond

namespace VideoDisplay {


  class AudioTransfer;

  /// Base class for video file inputs, for the VideoDisplay framework
  class VIDEODISPLAY_API VideoIn : public Radiant::Thread
  {
  public:

    /*
      unused now.

    enum {
      MAX_AUDIO_CHANS = 5,
      MAX_AUDIO_SAMPLES_IN_FRAME = MAX_AUDIO_CHANS * 28000
    };
    */
    /// The type of frame
    enum FrameType {
      FRAME_INVALID,
      FRAME_IGNORE,
      FRAME_STREAM,
      FRAME_LAST,
      FRAME_SNAPSHOT
    };

    /** Request from the host. */
    enum Request {
      NO_REQUEST,
      START,
      SEEK,
      STOP,
      FREE_MEMORY
    };

    /** Video image, for use inside the VideoDisplay library. */
    class VIDEODISPLAY_API Frame : public Patterns::NotCopyable
    {
    public:
      Frame();
      ~Frame();

      void copyAudio(const void * audio, int channels, int frames,
                     Radiant::AudioSampleFormat format,
                     Radiant::TimeStamp ts);
      void skipAudio(Radiant::TimeStamp amount,
                     int channels, int samplerate);

      Radiant::VideoImage m_image;
      Radiant::TimeStamp m_time;
      Radiant::TimeStamp m_absolute;
      Radiant::TimeStamp m_audioTS;
      Radiant::TimeStamp m_lastUse;
      float   * m_audio;
      int       m_allocatedAudio;
      int       m_audioFrames;
      FrameType m_type;
    };

    /** Basic informationa about a video file. */
    class VIDEODISPLAY_API VideoInfo
    {
    public:
      VideoInfo()
    : m_videoFrameSize(0, 0),
      m_videoDurationSecs(0),
      m_fps(-1)
      {}

      Nimble::Vector2i m_videoFrameSize;
      double           m_videoDurationSecs;
      double           m_fps;
    };

    VideoIn();
    virtual ~VideoIn();


    Frame * getFrame(int i, bool updateCounter);

    virtual bool init(const char * filename,
                       Radiant::TimeStamp pos,
                       int flags);
    virtual bool play(Radiant::TimeStamp pos = -1);
    virtual void stop();
    virtual bool seek(Radiant::TimeStamp pos);
    virtual void freeUnusedMemory();
    // VIDEODISPLAY_API virtual void enableLooping(bool enable) = 0;

    virtual void getAudioParameters(int * channels,
                    int * sample_rate,
                    Radiant::AudioSampleFormat * format) = 0;

    virtual float fps() = 0;

    size_t latestFrame() const { return m_decodedFrames - 1; }
    bool atEnd();
    bool isFrameAvailable(int frame) const
    { return(int) m_decodedFrames > frame && frame >= 0;}

    /// Finds the closest frame to the given time
    int selectFrame(int starfrom, Radiant::TimeStamp time) const;
    size_t decodedFrames() const { return m_decodedFrames; }
    size_t frameRingBufferSize() const { return m_frames.size(); }

    virtual double durationSeconds() = 0;

    size_t finalFrames()   const { return m_finalFrames; }

    const char * name() { return m_name.c_str(); }

    static void setDebug(int level);
    static void toggleDebug();

    const VideoInfo & vdebug() const { return m_info; }

    void setAudioListener(AudioTransfer * listener);

    Radiant::Mutex & mutex() { return m_mutex; }

    Radiant::TimeStamp firstFrameTime() const { return m_firstFrameTime; }

    // todo: static void setDefaultLatency(float seconds) { m_defaultLatency = seconds; }

    inline bool atEnd() const { return m_atEnd; }
    inline Radiant::TimeStamp displayFrameTime() const { return m_displayFrameTime; }

  protected:

    virtual void childLoop () ;

    virtual bool open(const char * filename, Radiant::TimeStamp pos) = 0;

    bool playing() { return m_playing; }
    // Get snapshot of the video in the given position
    virtual void videoGetSnapshot(Radiant::TimeStamp pos) = 0;
    // Start playing the video in the given position
    virtual void videoPlay(Radiant::TimeStamp pos) = 0;
    // Get the next next frame
    virtual void videoGetNextFrame() = 0;
    // Stop the video
    virtual void videoStop() = 0;

    /** An implmentation should use the methods below: */
    void allocateFrames(size_t frameCount, size_t width, size_t height,
                                         Radiant::ImageFormat fmt);

    void deallocateFrames();

    Frame * putFrame(const Radiant::VideoImage *,
             FrameType type,
             Radiant::TimeStamp show,
             Radiant::TimeStamp absolute,
             bool immediate);

    void ignorePreviousFrames();
    void freeFreeableMemory();

    /// @cond
    class Req
    {
    public:
      Req(Request r = NO_REQUEST, Radiant::TimeStamp time = 0)
          : m_request(r), m_time(time) {}
      volatile Request   m_request;
      Radiant::TimeStamp m_time;
    };

    enum {
      REQUEST_QUEUE_SIZE = 32
    };

    std::vector<std::shared_ptr<Frame> > m_frames;

    VideoInfo m_info;

    volatile size_t m_decodedFrames;
    volatile size_t m_consumedFrames;
    volatile size_t m_consumedAuFrames;
    volatile size_t m_finalFrames;

    volatile bool m_breakBack;
    volatile bool m_playing;

    int m_flags;
    int m_channels;
    int m_sample_rate;
    Radiant::AudioSampleFormat m_auformat;

    size_t m_auBufferSize;
    size_t m_auFrameBytes;

    volatile bool m_continue;

    Radiant::Condition m_vcond;
    Radiant::Mutex m_vmutex;

    Radiant::Condition m_acond;
    Radiant::Mutex m_amutex;

    float          m_fps;
    bool           m_done;
    bool           m_ending;
    bool           m_decoding;
    bool           m_atEnd;

    std::string    m_name;

    static int     m_debug;

    volatile unsigned m_consumedRequests;
    volatile unsigned m_queuedRequests;
    Req               m_requests[REQUEST_QUEUE_SIZE];
    Radiant::Mutex m_requestMutex;

    Radiant::TimeStamp m_frameTime;
    Radiant::TimeStamp m_displayFrameTime;

    AudioTransfer     *m_listener;

    Radiant::Mutex m_mutex;

    Radiant::TimeStamp m_firstFrameTime;

    /// @endcond

  private:
    /// Disabled
    VideoIn(const VideoIn & ) : Radiant::Thread() {}
    void pushRequest(const Req & r);


  };

}

/// @endcond

#endif
