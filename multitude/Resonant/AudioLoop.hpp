/* COPYRIGHT
 *
 * This file is part of Resonant.
 *
 * Copyright: MultiTouch Oy, Helsinki University of Technology and others.
 *
 * See file "Resonant.hpp" for authors and more details.
 *
 * This file is licensed under GNU Lesser General Public
 * License (LGPL), version 2.1. The LGPL conditions can be found in 
 * file "LGPL.txt" that is distributed with this source package or obtained 
 * from the GNU organization (www.gnu.org).
 * 
 */

#ifndef RESONANT_AUDIO_LOOP_HPP
#define RESONANT_AUDIO_LOOP_HPP

#include <Patterns/NotCopyable.hpp>

#include <Resonant/Export.hpp>

#include <string>

namespace Resonant {

  /** A simple audio IO class.

      This class uses PortAudio to handle the real work. It is basically a
      utility class that hides all the PortAudio code behind a bit easier API.
  */

  class RESONANT_API AudioLoop : public Patterns::NotCopyable
  {
  public:
    /// Creates a new audio IO object
    AudioLoop();
    /// Deletes an audio OI object
    virtual ~AudioLoop();

    /// Start the AudioLoop.
    /** In pratice this spans a new thread that is managed by the PortAudio (or rather,
        the operating system audio engine). */
    bool startReadWrite(int samplerate, int channels);
    /// Check if the audio IO is operational
    inline bool isRunning() { return m_isRunning; }

    /// Stop the audio processing
    bool stop();

    /// Returns the number of output channels in the current setup.
    /** This number reflects the numer of channels that the current sound
        card has. It is quite typical for sound cards to advertise more channels
        they actually have. This may be caused by a sound-card manufacturer
        using the same chips in two sounds cards, with a one card having 10
        DACs, while the other might have only 4 (case with M-Audio delta 44 vs 1010).

    */
    size_t outChannels() const;

    static void setDevicesFile(const std::string & xmlFilename);

  protected:
    /// This is called from PortAudio thread when the stream becomes inactive
    /// @param streamid Device / Stream number, @see setDevicesFile()
    virtual void finished(int streamid);

    virtual int callback(const void * in, void * out,
                         unsigned long framesPerBuffer, int streamid

//        , const PaStreamCallbackTimeInfo* time,
//        PaStreamCallbackFlags status
        ) = 0;

    bool       m_isRunning;
    bool       m_initialized;

    class AudioLoopInternal;

    AudioLoopInternal * m_d;
 };

}


#endif

