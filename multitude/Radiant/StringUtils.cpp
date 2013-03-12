/* Copyright (C) 2007-2013: Multi Touch Oy, Helsinki University of Technology
 * and others.
 *
 * This file is licensed under GNU Lesser General Public License (LGPL),
 * version 2.1. The LGPL conditions can be found in file "LGPL.txt" that is
 * distributed with this source package or obtained from the GNU organization
 * (www.gnu.org).
 * 
 */

#include "StringUtils.hpp"

#ifdef WIN32
#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
// #include <WinPort.h>
#endif

#ifdef __GNUC__
#include <cxxabi.h>
#endif

namespace Radiant
{

  namespace StringUtils
  {

    void eraseNonVisibles(QString & s)
    {
      QChar* data = s.data();
      int len = s.size();
      unsigned int out = 0;
      for(int in = 0; in < len; ++in) {
        if(data[in] < 32) {
          --len;
        } else {
          data[out++] = data[in];
        }
      }
      s.resize(out);
    }

    QByteArray demangle(const char * name)
    {
#ifdef __GNUC__
      int status = 0;
      char * tmp = abi::__cxa_demangle(name, 0, 0, &status);
      if(status == 0 && tmp) {
        QByteArray ret(tmp);
        free(tmp);
        return ret;
      }
#endif
      return name;
    }

#ifdef WIN32
    QString getLastErrorMessage()
    {
      const int   errStrSize = 1024;
      char  szErrStr[errStrSize] = {'\0'};
      FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, 0,
        GetLastError(), MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), szErrStr, errStrSize, 0);

      return QString::fromAscii(szErrStr);
    }
#endif

  }

}

