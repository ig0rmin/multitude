/* COPYRIGHT
 *
 * This file is part of Nimble.
 *
 * Copyright: MultiTouch Oy, Helsinki University of Technology and others.
 *
 * See file "Nimble.hpp" for authors and more details.
 *
 * This file is licensed under GNU Lesser General Public
 * License (LGPL), version 2.1. The LGPL conditions can be found in 
 * file "LGPL.txt" that is distributed with this source package or obtained 
 * from the GNU organization (www.gnu.org).
 * 
 */

#ifndef NIMBLE_RANDOM_HPP
#define NIMBLE_RANDOM_HPP

#include "Export.hpp"
#include "Rect.hpp"

#include <stdint.h>

#include <random>

namespace Nimble {

  /// Random number generator with uniform distribution
  /** This class generates random numbers with uniform
      distribution.

      It uses a fast and cheap modulo-based random number
      generation. The side effext of this is that the lower bits of
      the random number sequence are not so very random. If you need
      small integers with random values you will need to use the
      #rand24 function, which uses only the 24 higher bits that are
      fairly random.

      The random number sequence is identical on all platforms, given
      the same seed value.
  */
  class NIMBLE_API RandomUniform
  {
  public:
    /// Constructs a new random number generator with the given seed value
    RandomUniform(uint32_t val = 0) : m_rand((unsigned long)val) { }
    ~RandomUniform() {}

    /// Random numbers between 0 and 1
    inline float rand01()
    {
      uint32_t tmp = m_dist(m_rand);
      return (float) tmp * (1.0f / (float) ((uint32_t) 0xffffffff));
    }

    /// Random numbers between 0 and x
    inline float rand0X(float x)
    {
      uint32_t tmp = m_dist(m_rand);
      return (float) tmp * (x / (float) ((uint32_t) 0xffffffff));
    }

    /// Random numbers between 0 and x
    inline double rand0X(double x)
    {
      uint32_t tmp = m_dist(m_rand);
      return (double) tmp * (x / (double) ((uint32_t) 0xffffffff));
    }

    /// Random numbers between 0 and x-1
    inline uint32_t rand0X(uint32_t x)
    {
      return rand32() % x;
    }

    /// Random numbers between 0 and x-1
    inline uint64_t rand0X64(uint64_t x)
    {
      uint64_t tmp1 = rand();
      uint64_t tmp2 = rand();

      return (tmp1 | (tmp2 << 32)) % x;
    }

    /// Random numbers between -1 and 1
    inline float rand11()
    {
      uint32_t tmp = m_dist(m_rand);
      return (float) tmp * (2.0f / (float) ((uint32_t) 0xffffffff)) - 1.0f;
    }

    /// Random numbers between -x and x
    inline float randXX(float x)
    {
      uint32_t tmp = m_dist(m_rand);
      return (float) tmp * (2.0f * x / (float) ((uint32_t) 0xffffffff)) - x;
    }

    /// Random numbers between min and max
    inline float randMinMax(float min, float max)
    {
      return rand0X(max - min) + min;
    }

    /// A random number in range 0-2^32. The lower bits of the random
    /// number are not totally random.
    /// @return Generated random number
    inline uint32_t rand()
    {
      return m_dist(m_rand);
    }

    /// A random number in range 0-2^24. All bits of the value should
    /// be fairly random.
    /// @return Generated random number
    inline uint32_t rand24()
    {
      return m_dist(m_rand) >> 8;
    }

    /// A random number in range 0-2^32. All bits of the value should
    /// be fairly random.
    /// @return Generated random number
    inline uint32_t rand32()
    {
      /* Generate two random numbers are take the 16 higher bits from
         both. */
      return m_dist(m_rand);
    }

    /// Get random numbers between 0 and range-1.
    /// @param range The maximum output value. This value must not exceed 2^24-1.
    /// @return Generated random number
    inline uint32_t randN24(uint32_t range)
    {
      return m_dist(m_rand);
    }

    /// Random 2d vector inside a rectangle
    inline Nimble::Vector2f randVec2InRect(const Nimble::Rectf & r)
    {
      return Nimble::Vector2f(randMinMax(r.low().x, r.high().x),
                              randMinMax(r.low().y, r.high().y));
    }

    /// Random 2d vector on a unit circle
    inline Nimble::Vector2f randVecOnCircle(float radius = 1.0f)
    {
      float a = rand0X(Math::TWO_PI);
      return Nimble::Vector2f(cosf(a) * radius, sinf(a) * radius);
    }

    /// Random 2d vector on a unit circle
    inline Nimble::Vector2f randVecInCircle(float radius = 1.0f)
    {
      while(true) {
        Nimble::Vector2f v(rand11(), rand11());
        if(v.lengthSqr() > 1.0f)
          continue;

        v *= radius;
        return v;
      }
    }

    /// Random boolean
    /// @todo does this work with 32-bit computers too?
    /// @return True or false.
    inline bool randBool()
    {
      // count bits in 13 bit random value
      return std::uniform_int_distribution<uint32_t>(0,1)(m_rand);
    }

    /// Returns a reference to an instance
    static RandomUniform & instance() { return m_instance; }

    /// @todo add static members inside Nimble::Math ?

  private:
    std::mt19937 m_rand;
    std::uniform_int_distribution<uint32_t> m_dist;
    static RandomUniform  m_instance;
  };

  /// RandomGaussian generates pseudo-random numbers from a normal (gaussian) distribution.
  class NIMBLE_API RandomGaussian
  {
    public:
      /// Construct a generator with given parameters for the distribution of
      /// the random numbers.
      /// @param mean the mean of the normal distribution
      /// @param stdDev the standard deviation for the normal distribution
      /// @param seed seed value for the pseudo-random sequence
      RandomGaussian(float mean = 0.0f, float stdDev = 1.0f, uint32_t seed = 0) : m_uniform(seed), m_mean(mean), m_stdDev(stdDev) {}

      /// Generate a random number from the distribution
      /// @return a pseudo-random number
      inline float rand() {
        float x1, x2, rsq;

        do {
          // Pick two uniform numbers within a unit-square and test if they are
          // within a unit-circle, if not, try again
          x1 = 2.0f * m_uniform.rand01() - 1.0f;
          x2 = 2.0f * m_uniform.rand01() - 1.0f;
          rsq = x1 * x1 + x2 * x2;
        } while(rsq >= 1.0f || rsq == 0.0f);

        // Box-Muller transformation and return the other number
        float fac = sqrt((-2.0f * log(rsq)) / rsq);
        return (x2 * fac) * m_stdDev + m_mean;
      }

    private:
      RandomUniform m_uniform;
      float m_mean;
      float m_stdDev;
  };
}

#endif
