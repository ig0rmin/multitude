#include "Timer.hpp"

#if defined(WIN32)
# define WIN32_LEAN_AND_MEAN
# include <Windows.h>
#else
# include <sys/time.h>
#endif

namespace Radiant
{

  class Timer::D {
  public:
#if defined(WIN32)
    LARGE_INTEGER m_performanceFrequency;
    double m_performanceReciprocal;

    LARGE_INTEGER m_startTime;
#else
    struct timeval m_startTime;
#endif
  };

  //////////////////////////////////////////////////////////////////////
  //////////////////////////////////////////////////////////////////////

  Timer::Timer() : m_d(new D)
  {
#if defined(WIN32)
    QueryPerformanceFrequency(&m_d->m_performanceFrequency);
    m_d->m_performanceReciprocal = 1.0 / static_cast<double> (m_d->m_performanceFrequency.QuadPart);
#endif
    start();
  }

  Timer::Timer(const Timer & timer) :
    m_d(new D)
  {
    *m_d = *timer.m_d;
  }

  Timer::~Timer()
  {
    delete m_d;
  }

  void Timer::start()
  {
#if defined(WIN32)
    QueryPerformanceCounter(&m_d->m_startTime);
#else
    gettimeofday(&m_d->m_startTime, 0);
#endif
  }

  int Timer::resolution() const
  {
#if defined(WIN32)
    return static_cast<int> (m_d->m_performanceFrequency.QuadPart);
#else
    /// gettimeofday gives microsecond resolution (in theory)
    return static_cast<int> (1e6);
#endif
  }

  float Timer::time() const
  {
#if defined(WIN32)
    LARGE_INTEGER endTime;

    QueryPerformanceCounter(&endTime);

    return float((endTime.QuadPart - m_d->m_startTime.QuadPart) * m_d->m_performanceReciprocal);
#else
    struct timeval endTime;

    gettimeofday(&endTime, 0);

    float ds  = endTime.tv_sec - m_d->m_startTime.tv_sec;
    float dus = endTime.tv_usec - m_d->m_startTime.tv_usec;

    return ds + 1e-6f * dus;
#endif
  }

}
