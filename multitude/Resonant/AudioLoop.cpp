/* Copyright (C) 2007-2013: Multi Touch Oy, Helsinki University of Technology
 * and others.
 *
 * This file is licensed under GNU Lesser General Public License (LGPL),
 * version 2.1. The LGPL conditions can be found in file "LGPL.txt" that is
 * distributed with this source package or obtained from the GNU organization
 * (www.gnu.org).
 * 
 */

#include "AudioLoop.hpp"
#include "Resonant.hpp"
#include "AudioLoop_private.hpp"

#include <Nimble/Math.hpp>

#include <Radiant/Sleep.hpp>
#include <Radiant/Trace.hpp>
#include <Radiant/StringUtils.hpp>

#include <Valuable/Serializer.hpp>
#include <Valuable/AttributeContainer.hpp>

#include <string>

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define FRAMES_PER_BUFFER 128

namespace {
  QString s_xmlFilename;
}

namespace Resonant {
  using Radiant::FAILURE;

  AudioLoop::AudioLoop()
    : m_isRunning(false),
    m_initialized(false)
  {
    m_d = new AudioLoopInternal();

    // Initializes PortAudio / increases the usage counter.
    PaError e = Pa_Initialize();
    if(e == paNoError) {
      m_initialized = true;
    } else {
      Radiant::error("AudioLoop::init # %s", Pa_GetErrorText(e));
    }
  }

  AudioLoop::~AudioLoop()
  {
    if (isRunning())
      Radiant::error("AudioLoop::~AudioLoop(): audio still running");

    delete m_d;

    if(m_initialized) {
      // Decreases usage counter, if this is the last AudioLoop using PA, the library is closed.
      // From Pa API: Pa_Terminate() MUST be called before exiting a program
      //              which uses PortAudio. Failure to do so may result in
      //              serious resource leaks, such as audio devices not being
      //              available until the next reboot.
      PaError e = Pa_Terminate();
      if(e != paNoError) {
        Radiant::error("AudioLoop::cleanup # %s", Pa_GetErrorText(e));
      }
    }
  }

  size_t AudioLoop::outChannels() const
  {
    return m_d->m_channels.size();
  }

  void AudioLoop::setDevicesFile(const QString & xmlFilename)
  {
    s_xmlFilename = xmlFilename;
  }

  bool AudioLoop::startReadWrite(int samplerate, int channels)
  {
    assert(!isRunning());

    const char * chankey = getenv("RESONANT_OUTCHANNELS");
    int forcechans = -1;
    if(chankey != 0) {
      forcechans = atoi(chankey);
    }

    /// List of device/channel request pairs
    /// For example [("Sound Blaster 1", 3), ("Turtle Beach", 2), (7, 1)]
    /// means that channels 0..2 will be mapped to Sound Blaster channels 0..2,
    /// channels 3..4 are mapped to Turtle Beach channels 0..1, and channel 5 is mapped
    /// to sound device number 7 channel 0.
    /// It seems that portaudio doesn't allow opening any random channels devices,
    /// just n first channels.
    typedef std::pair<QString, int> Device;
    typedef std::vector<Device> Devices;
    Devices devices;

    const char * devname = getenv("RESONANT_DEVICE");
    if(devname) {
      devices.push_back(Device(devname, channels));
    } else if(!s_xmlFilename.isEmpty()) {
      devices = *Valuable::Serializer::deserializeXML<Valuable::AttributeContainer<Devices> >(s_xmlFilename);
    }

    if(devices.empty()) {
      m_d->m_streams.push_back(Stream());
      Stream & s = m_d->m_streams.back();

      s.outParams.device = Pa_GetDefaultOutputDevice();
      if(s.outParams.device == paNoDevice) {
        for (int i = 0; i < Pa_GetDeviceCount();++ i) {
          const PaDeviceInfo * info = Pa_GetDeviceInfo(i);
          if (QByteArray("default") == info->name) {
            s.outParams.device = i;
            break;
          }
        }

        if (s.outParams.device == paNoDevice) {
          if (Pa_GetDeviceCount() > 0) {
            s.outParams.device = 0;
          } else {
            Radiant::error("AudioLoop::startReadWrite # No default output device available");
            return false;
          }
        }
      }

      devices.push_back(Device("", channels));

      debugResonant("AudioLoop::startReadWrite # Selected default output device %d", s.outParams.device);
    }
    else {
      for (size_t dev = 0; dev < devices.size(); ++dev) {
        m_d->m_streams.push_back(Stream());
        Stream & s = m_d->m_streams.back();

        QByteArray tmp = devices[dev].first.toUtf8();
        const char * devkey = tmp.data();
        int channel_requests = devices[dev].second;
        char * end = 0;
        int i = strtol(devkey, & end, 10);

        long decoded = end - devkey;
        if(decoded == (long) strlen(devkey)) {
          s.outParams.device = i;
          debugResonant("AudioLoop::startReadWrite # Selected device %d (%s)", (int) s.outParams.device, devkey);
        }
        else {
          int n = Pa_GetDeviceCount();

          for( i = 0; i < n; i++) {
            const PaDeviceInfo * info = Pa_GetDeviceInfo(i);
            if(strstr(info->name, devkey) != 0) {
              if (channel_requests > info->maxOutputChannels) {
                debugResonant("Skipping device %d, not enough output channels (%d < %d)",
                              (int)dev, info->maxOutputChannels, channel_requests);
                continue;
              }

              s.outParams.device = i;

              debugResonant("AudioLoop::startReadWrite # Selected device %d %s",
                            (int) s.outParams.device, info->name);
              break;
            }
          }
          if (i == n) {
            Radiant::error("Couldn't find device %s", devkey);
          }
        }
      }
    }

    for (size_t streamnum = 0, streams = m_d->m_streams.size(); streamnum < streams; ++streamnum) {
      Stream & s = m_d->m_streams[streamnum];
      /// @todo m_barrier isn't released ever
      s.m_barrier = streams == 1 ? 0 : new Radiant::Semaphore(0);
      channels = devices[streamnum].second;

      const PaDeviceInfo * info = Pa_GetDeviceInfo(s.outParams.device);

      debugResonant("AudioLoop::startReadWrite # Got audio device %d = %s",
                    (int) s.outParams.device, info->name);

      if(Radiant::enabledVerboseOutput()) {
        int n = Pa_GetDeviceCount();

        for(int i = 0; i < n; i++) {
           const PaDeviceInfo * info2 = Pa_GetDeviceInfo(i);
           const PaHostApiInfo * apiinfo = Pa_GetHostApiInfo(info2->hostApi);
          debugResonant("AudioLoop::startReadWrite # Available %d: %s (API = %s)",
                        i, info2->name, apiinfo->name);
        }
      }

      // int minchans = std::min(info->maxInputChannels, info->maxOutputChannels);
      int minchans = info->maxOutputChannels;

      if(forcechans > 0) {
        channels = forcechans;
      }
      else if(minchans < channels || (channels != minchans)) {
        debugResonant("AudioLoop::startReadWrite # Expanding to %d channels",
                       minchans);
        channels = minchans;
      }

      debugResonant("AudioLoop::startReadWrite # channels = %d limits = %d %d",
                     channels, info->maxInputChannels, info->maxOutputChannels);


      s.outParams.channelCount = channels;
      s.outParams.sampleFormat = paFloat32;
      s.outParams.suggestedLatency =
        Pa_GetDeviceInfo( s.outParams.device )->defaultLowOutputLatency;
      s.outParams.hostApiSpecificStreamInfo = 0;

      s.inParams = s.outParams;
      s.inParams.device = Pa_GetDefaultInputDevice();

      m_d->cb.push_back(std::make_pair(this, static_cast<int> (streamnum)));

      PaError err = Pa_OpenStream(& s.stream,
                                  0, // & m_inParams,
                                  & s.outParams,
                                  samplerate,
                                  FRAMES_PER_BUFFER,
                                  paClipOff,
								  &AudioLoopInternal::paCallback,
                                  &m_d->cb.back() );

      if( err != paNoError ) {
        Radiant::error("AudioLoop::startReadWrite # Pa_OpenStream failed (device %d, channels %d, sample rate %d)",
                       s.outParams.device, channels, samplerate);
        return false;
      }

      err = Pa_SetStreamFinishedCallback(s.stream, & m_d->paFinished );

      s.streamInfo = Pa_GetStreamInfo(s.stream);

      for (int i = 0; i < s.outParams.channelCount; ++i)
        m_d->m_channels[static_cast<int> (m_d->m_channels.size())] = Channel(static_cast<int> (streamnum), i);

      debugResonant("AudioLoop::startReadWrite # %d channels lt = %lf, EXIT OK",
         (int) s.outParams.channelCount,
         (double) s.streamInfo->outputLatency);
    }

    m_d->m_streamBuffers.resize(m_d->m_streams.size());
    m_d->m_sem.release(static_cast<int> (m_d->m_streams.size()));

    m_isRunning = true;

    for (size_t streamnum = 0; streamnum < m_d->m_streams.size(); ++streamnum) {
      Stream & s = m_d->m_streams[streamnum];

      PaError err = Pa_StartStream(s.stream);

      if( err != paNoError ) {
        Radiant::error("AudioLoop::startReadWrite # Pa_StartStream failed");
        return false;
      }

      s.startTime = Pa_GetStreamTime(s.stream);
    }

    return true;
  }

  bool AudioLoop::stop()
  {
    if(!isRunning())
      return true;

    m_isRunning = false;

    {
      /* Hack to get the audio closed in all cases (mostly for Linux). */
      Radiant::Sleep::sleepMs(200);
    }

    for (size_t num = 0; num < m_d->m_streams.size(); ++num) {
      Stream & s = m_d->m_streams[num];
      int err = Pa_CloseStream(s.stream);
      if(err != paNoError) {
        Radiant::error("AudioLoop::stop # Could not close stream");
      }
      s.stream = 0;
      s.streamInfo = 0;
      m_d->m_channels.erase(static_cast<int> (num));
    }

    return true;
  }

  int AudioLoop::AudioLoopInternal::paCallback(const void *in, void *out,
                unsigned long framesPerBuffer,
                const PaStreamCallbackTimeInfo * time,
                PaStreamCallbackFlags status,
                void * self)
  {
    std::pair<AudioLoop*, int> stream = *reinterpret_cast<std::pair<AudioLoop*, int>*>(self);

    int r = stream.first->callback(in, out, framesPerBuffer, stream.second, *time, status);
    return stream.first->m_isRunning ? r : paComplete;
  }

  void AudioLoop::AudioLoopInternal::paFinished(void * self)
  {
    std::pair<AudioLoop*, int> stream = *reinterpret_cast<std::pair<AudioLoop*, int>*>(self);
    stream.first->finished(stream.second);
    debugResonant("AudioLoop::paFinished # %p %d", stream.first, stream.second);
  }


  void AudioLoop::finished(int /*streamid*/)
  {
    m_isRunning = false;
  }

}
