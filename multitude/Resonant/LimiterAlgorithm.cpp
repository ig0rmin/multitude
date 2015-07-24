#include "LimiterAlgorithm.hpp"

#include <Radiant/Trace.hpp>

#include <Nimble/Math.hpp>
#include <cmath>

namespace Resonant {

  float ChannelLimiter::putGet(float insample,
                               float thresholdLog,
                               unsigned attackTime,
                               unsigned releaseTime)
  {
    // Store pure data to delay buffer:
    m_buffer.put(insample);

    // Store decibels to delay buffer:
    insample = std::abs(insample);

    if(insample < 1e-10) insample = 1e-10;

    float insampleLog = std::max<float>(std::log(insample), thresholdLog);

    m_logBuffer.put(insampleLog);

    int design = 0;

    // See if the new input value indicated new peak:
    if(m_untilPeak)
    {
      float requiredGain = thresholdLog - insampleLog;
      float ats = (float) attackTime - 1;
      float tmp2 = m_step * ats + m_gain;

      if(tmp2 > requiredGain) {
        // float oldStep = m_step;
        m_step = (requiredGain - m_gain) / ats;
        m_untilPeak = attackTime;

        /* MJ_trace2("Good redesign %f %f -> %f %f # %f", m_gain, oldStep,
     m_step, insampleLog, requiredGain);  */
        design = 1;
      }
    }

    if(m_untilPeak) m_untilPeak--;

    // Seek new attack path
    // if(true) {
    if(!design) {
      // if(!m_untilPeak) {

      m_step = 0.0;

      for(unsigned i=1; i <= attackTime; i++) {
        float tmp = m_logBuffer.getNewest(attackTime - i);
        float requiredGain = thresholdLog - tmp;
        float is = (float) i;
        float tmp2 = m_step * is + m_gain;

        if(tmp2 > requiredGain) {
          m_step = (requiredGain - m_gain) / is;
          m_untilPeak = i - 1;
          design = 2;
          /* MJ_trace2("Poor redesign %f %d # %f %f",
       m_gain + m_step * (m_untilPeak + 1),
       m_untilPeak, tmp, requiredGain); */

        }
      }
    }

    float delayedSample = m_buffer.getNewest(attackTime-1);
    m_level.put(m_logBuffer.getNewest(attackTime-1), thresholdLog, releaseTime);

    if(!design) {
      float tmp = m_level.peak();
      float requiredGain = thresholdLog - tmp;
      m_step = (requiredGain - m_gain) / (float) releaseTime;
      design = 3;
      // m_mul = MJ_pow(requiredGain / m_gain, 1.0 / (float) releaseTime);
    }

    m_gain += m_step;

    //      printf(" [inLog = %f %f %d]\n", insampleLog, m_gain, design);//  fflush(0);

    // MJ_fatal(MJ_ERR_UNKNOWN, "FOO"); // Halt here
    float gainLinear = expf(m_gain);
    float rval = delayedSample * gainLinear;

    float outLog = std::log(std::abs(rval));

    if(outLog > (thresholdLog+.001) || !Nimble::Math::isFinite(rval)){
      float gainDiff = thresholdLog - outLog;

      Radiant::info("END vals %f %f %f %f < %f %f %f # %d %d %d", m_gain,
                    m_step, gainDiff, thresholdLog,
                    outLog, m_logBuffer.getNewest(attackTime-1), insampleLog,
                    m_untilPeak, attackTime, design);
      Radiant::fatal("MJ_ChannelLimiter::putGet");
    }

    return rval;
  }


  }
