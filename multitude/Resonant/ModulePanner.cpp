/* Copyright (C) 2007-2013: Multi Touch Oy, Helsinki University of Technology
 * and others.
 *
 * This file is licensed under GNU Lesser General Public License (LGPL),
 * version 2.1. The LGPL conditions can be found in file "LGPL.txt" that is
 * distributed with this source package or obtained from the GNU organization
 * (www.gnu.org).
 * 
 */

#include "ModulePanner.hpp"
#include "Resonant.hpp"

#include <Nimble/Interpolation.hpp>

#include <Radiant/BinaryData.hpp>
#include <Radiant/Trace.hpp>

#include <Valuable/DOMElement.hpp>
#include <Valuable/DOMDocument.hpp>
#include <Valuable/Attribute.hpp>

#include <assert.h>
#include <string.h>

namespace Resonant {

  ModulePanner::ModulePanner(Mode mode)
      : Module(),
      m_speakers(this, "speakers"),
      m_generation(0),
      m_maxRadius(this, "max-radius", 1500),
      m_rectangles(this, "rectangles"),
      m_operatingMode(this, "mode", mode)
  {
    setName("pan2d");

  }

  ModulePanner::~ModulePanner()
  {
    for (size_t i=0; i < m_rectangles->size(); ++i)
      delete (*m_rectangles)[i];
  }

  bool ModulePanner::deserialize(const Valuable::ArchiveElement & element)
  {
    m_rectangles->clear();
    m_speakers->clear();
    bool ok = Module::deserialize(element);
    ++m_generation;
    updateChannelCount();

    return ok;
  }

  bool ModulePanner::prepare(int & channelsIn, int & channelsOut)
  {
    (void) channelsIn;

    channelsOut = static_cast<int> (m_channelCount);

    return true;
  }

  void ModulePanner::eventProcess(const QByteArray & id,
                                    Radiant::BinaryData & data)
  {
    debugResonant("ModulePanner::control # %s", id.data());

    bool ok = true;

    if(id == "fullhdstereo") {
      makeFullHDStereo();
    }
    else if(id == "addsource") {
      Source s;
      data.readString(s.m_id);
      m_sources.push_back(s);
    }
    else if(id == "removesource") {
      QByteArray id;
      data.readString(id);
      removeSource(id);
    }
    else if(id == "setsourcelocation") {
      QByteArray id = data.read<QByteArray>();
      QByteArray path = data.read<QByteArray>();
      Nimble::Vector2 loc = data.readVector2Float32( & ok);

      if(ok) {
        setSourceLocation(id, path, loc);
      }
      else {
        Radiant::error("ModulePanner::control # %s # Could not read source location",
              id.data());
      }
    }
    else if(id == "clearsourcelocation") {
      QByteArray id = data.read<QByteArray>();
      QByteArray path = data.read<QByteArray>(&ok);

      if (ok) {
        clearSourceLocation(id, path);
      } else {
        Radiant::error("ModulePanner::control # %s # Could not parse command clearsourcelocation",
                       id.data());
      }
    }
    else {
      Radiant::error("ModulePanner::control # Unknown command %s", id.data());
    }
  }

  void ModulePanner::process(float ** in, float ** out, int n, const CallbackTime &)
  {
    int bufferbytes = n * sizeof(float);

    // Zero the output channels
    for(unsigned i = 0; i < m_channelCount; i++) {
      memset(out[i], 0, bufferbytes);
    }

    for(int i = 0; i < (int) m_sources.size(); i++) {

      Source & s = *m_sources[i];

      for(int j = 0; j < (int) s.m_pipes.size(); j++) {

        Pipe & p = s.m_pipes[j];

        if(p.isDone())
          continue;

        const float * src = in[i];

        float * dest = out[p.m_to];
        float * sentinel = dest + n;

        if(p.m_ramp.left()) {

          for( ; dest < sentinel; dest++, src++) {
            *dest += (*src * p.m_ramp.value());
            p.m_ramp.update();
          }
        }
        else {
          float v = p.m_ramp.value();
          for( ; dest < sentinel; dest++, src++) {
            *dest += (*src * v);
          }
        }

        debugResonant("ModulePanner::process # source %d, pipe %d -> %d, gain = %f "
              "in = %p %f out = %f",
              i, j, p.m_to, p.m_ramp.value(), in[i], *in[i], out[p.m_to][0]);

      }
    }
  }

  void ModulePanner::makeFullHDStereo()
  {
    m_speakers->clear();

    LoudSpeaker * ls = new LoudSpeaker;

    ls->m_location = Nimble::Vector2f(0, 540);
    m_speakers->push_back(std::shared_ptr<LoudSpeaker>(ls));

    ls = new LoudSpeaker;

    ls->m_location = Nimble::Vector2f(1920, 540);
    m_speakers->push_back(std::shared_ptr<LoudSpeaker>(ls));

    m_maxRadius = 1200;
    ++m_generation;
    updateChannelCount();
  }

  void ModulePanner::addSoundRectangle(SoundRectangle * r)
  {
    //  Radiant::info("ModuleRectPanner::addSoundRectangle # new rect %d,%d %d,%d", r.location().x, r.location().y, r.size().x, r.size().y);

    m_rectangles->push_back(r);
  }

  void ModulePanner::setMode(Mode mode)
  {
    m_operatingMode = mode;
  }

  ModulePanner::Mode ModulePanner::getMode() const
  {
    return (ModulePanner::Mode)*m_operatingMode;
  }

  int ModulePanner::locationToChannel(Nimble::Vector2 location) const
  {
    if (*m_operatingMode == RADIAL) {
      int best = 0;
      float bestd = std::numeric_limits<float>::infinity();
      int i = 0;
      for(auto it = m_speakers->begin(); it != m_speakers->end(); it++, i++) {
        if (!*it) continue;
        const LoudSpeaker & l = **it;
        const float dist = (l.m_location - location).length();
        if (dist < bestd) {
          best = i;
          bestd = dist;
        }
      }

      return best;
    } else {
      const SoundRectangle * best = nullptr;
      float bestd = 0;

      for (const SoundRectangle * r: *m_rectangles) {
        float dist = r->rect().cast<float>().distance(location);
        if (!best || dist < bestd) {
          best = r;
          bestd = dist;
        }
      }

      if (best) {
        const float rectMidX = best->rect().cast<float>().center().x;
        return location.x < rectMidX ? best->leftChannel() : best->rightChannel();
      } else {
        return 0;
      }
    }
  }

  void ModulePanner::setSourceLocation(const QByteArray & id,
                                       const QByteArray & path,
                                       Nimble::Vector2 location)
  {
    debugResonant("ModulePanner::setSourceLocation # %s [%f %f]", id.data(),
          location.x, location.y);

    Source * s = 0;

    for(unsigned i = 0; i < m_sources.size(); i++) {
      Source & s2 = * m_sources[i];
      if(s2.m_id == id) {
        s = & s2;
      }
    }

    if(!s) {
      Radiant::error("ModulePanner::setSourceLocation # id \"%s\" is not known",
            id.data());
      return;
    }

    auto it = s->m_locations.find(path);
    if (it == s->m_locations.end()) {
      s->m_locations.insert(std::make_pair(path, location));
    } else if (s->m_generation == m_generation && it->second == location) {
      return;
    } else {
      it->second = location;
    }

    s->m_generation = m_generation;
    syncSource(*s);
  }

  void ModulePanner::clearSourceLocation(const QByteArray & id, const QByteArray & path)
  {
    for (auto & ptr: m_sources) {
      Source & s = *ptr;
      if (s.m_id == id) {
        auto it = s.m_locations.find(path);
        if (it == s.m_locations.end())
          return;

        s.m_locations.erase(it);
        s.m_generation = m_generation;
        syncSource(s);
        return;
      }
    }
  }

  void ModulePanner::syncSource(ModulePanner::Source & src)
  {
    int interpSamples = 2000;

    for (unsigned channel = 0; channel < m_channelCount; ++channel) {
      float gain = 0;
      /// If the audio source is played from in several different locations at
      /// the same time, always the the maximum gain for each speaker.
      /// Alternatively we could add up the gains, but then things like
      /// recursive ViewWidget would have very loud audio.
      for (auto & p: src.m_locations)
        gain = std::max(gain, computeGain(channel, p.second));

      if(gain <= 0.0000001f) {

        // Silence that output:
        for(unsigned j = 0; j < src.m_pipes.size(); j++) {
          Pipe & p = src.m_pipes[j];
          if(p.m_to == channel && p.m_ramp.target() >= 0.0001f) {
            p.m_ramp.setTarget(0.0f, interpSamples);
            debugResonant("ModulePanner::syncSource # Silencing %u", channel);

          }
        }
      }
      else {
        bool found = false;

        // Find existing pipe:
        for(unsigned j = 0; j < src.m_pipes.size() && !found; j++) {
          Pipe & p = src.m_pipes[j];

          debugResonant("Checking %u: %u %f -> %f", j, p.m_to,
                p.m_ramp.value(), p.m_ramp.target());

          if(p.m_to == channel) {
            debugResonant("ModulePanner::syncSource # Adjusting %u", j);
            p.m_ramp.setTarget(gain, interpSamples);
            found = true;
          }
        }

        if(!found) {

          // Pick up a new pipe:
          for(unsigned j = 0; j <= src.m_pipes.size() && !found; j++) {
            if(j == src.m_pipes.size()) {
              src.m_pipes.resize(j+1);
              debugResonant("ModulePanner::syncSource # pipes resize to %d", j+1);
            }
            Pipe & p = src.m_pipes[j];
            if(p.isDone()) {
              debugResonant("ModulePanner::syncSource # "
                    "Starting %u towards %u", j, channel);
              p.m_to = channel;
              p.m_ramp.setTarget(gain, interpSamples);
              found = true;
            }
          }
        }

        if(!found) {
          Radiant::error("Could not allocate pipe for a moving source");
        }
      }
    }
  }

  void ModulePanner::removeSource(const QByteArray & id)
  {

    for(Sources::iterator it = m_sources.begin(); it != m_sources.end(); ++it) {
      Source & s = * (*it);
      if(s.m_id == id) {
        m_sources.erase(it);
        debugResonant("ModulePanner::removeSource # Removed source %s, now %lu",
              id.data(), m_sources.size());
        return;
      }
    }

    Radiant::error("ModulePanner::removeSource # No such source: \"%s\"", id.data());
  }

  float ModulePanner::computeGain(unsigned int channel, Nimble::Vector2 srcLocation) const
  {
    switch(*m_operatingMode) {
    case RADIAL:
      return computeGainRadial(channel, srcLocation);
    case RECTANGLES:
      return computeGainRectangle(channel, srcLocation);
    default:
      return 0;
    }
  }

  float ModulePanner::computeGainRadial(unsigned int channel, Nimble::Vector2 srcLocation) const
  {
    if (channel >= m_speakers->size())
      return 0;

    const LoudSpeaker * ls = (*m_speakers)[channel].get();
    if (!ls)
      return 0;

    float d = (srcLocation - ls->m_location.asVector()).length();
    float rel = d / m_maxRadius;

    float inv = 1.0f - rel;

    return std::min(inv * 2.0f, 1.0f);
  }

  float ModulePanner::computeGainRectangle(unsigned int channel, Nimble::Vector2 srcLocation) const
  {
    float gain = 0.f;

    for (const SoundRectangle * r: *m_rectangles) {
      if (r->leftChannel() != (int)channel && r->rightChannel() != (int)channel)
        continue;

      //Radiant::info("ModuleRectPanner::computeGain # SPEAKER (%f,%f) source is inside rectangle (%d,%d) (%d,%d)", ls->m_location.x(), ls->m_location.y(), r->location().x, r->location().y, r->size().x, r->size().y);

      Nimble::Vector2 tmp(r->location().x, r->location().y);
      Nimble::Vector2 local = srcLocation - tmp;

      // Compute gain in y direction
      Nimble::LinearInterpolator<float> iy;
      iy.addKey(-r->fade(), 0.f);
      iy.addKey(0.f, 1.f);
      iy.addKey(r->size().y, 1.f);
      iy.addKey(r->size().y + r->fade(), 0.f);

      float gainY = iy.interpolate(local.y);

      // Compute gain in x direction
      Nimble::LinearInterpolator<float> ix;

      if (r->leftChannel() == r->rightChannel()) {
        ix.addKey(-r->fade(), 0.f);
        ix.addKey(0.f, 1.f);
        ix.addKey(r->size().x, 1.f);
        ix.addKey(r->size().x + r->fade(), 0.f);
      } else {
        if (r->leftChannel() == (int)channel) {
          // Left channel
          ix.addKey(-r->fade(), 0.f);
          ix.addKey(0.f, 1.f);
          ix.addKey(r->size().x, 1.f - r->stereoPan());
          ix.addKey(r->size().x + r->fade(), 0.f);
        } else {
          // Right channel
          ix.addKey(-r->fade(), 0.f);
          ix.addKey(0.f, 1.f - r->stereoPan());
          ix.addKey(r->size().x, 1.f);
          ix.addKey(r->size().x + r->fade(), 0.f);
        }
      }

      float gainX = ix.interpolate(local.x);

      //Radiant::info("ModuleRectPanner::computeGain # gain x %f, gain y %f", gainX, gainY);

      gain = std::max(gain, gainX * gainY);
    }

    // Radiant::info("ModuleRectPanner::computeGain # channel %u | source %.1f,%.1f | gain %.2f",
    //               channel, srcLocation.x, srcLocation.y, gain);

    return gain;
  }

  void ModulePanner::updateChannelCount()
  {
    if (*m_operatingMode == RADIAL) {
      m_channelCount = m_speakers->size();
    } else if (*m_operatingMode == RECTANGLES) {
      m_channelCount = 0;
      for (const SoundRectangle * rect: *m_rectangles)
        m_channelCount = std::max<unsigned int>(m_channelCount, std::max(rect->leftChannel(),
                                                                         rect->rightChannel()) + 1);
    } else {
      m_channelCount = 0;
    }
  }
}
