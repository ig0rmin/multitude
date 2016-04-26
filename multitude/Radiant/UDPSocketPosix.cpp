/* Copyright (C) 2007-2013: Multi Touch Oy, Helsinki University of Technology
 * and others.
 *
 * This file is licensed under GNU Lesser General Public License (LGPL),
 * version 2.1. The LGPL conditions can be found in file "LGPL.txt" that is
 * distributed with this source package or obtained from the GNU organization
 * (www.gnu.org).
 * 
 */

#include "UDPSocket.hpp"
#include "SocketWrapper.hpp"
#include "SocketUtilPosix.hpp"
#include "Trace.hpp"

#include <sys/types.h>

#ifdef RADIANT_LINUX
#include <sys/ioctl.h>
#endif

namespace Radiant
{

  class UDPSocket::D {
  public:
    D(int fd = -1) : m_fd(fd), m_port(0)
    {
    }

    int m_fd;
    int m_port;
    QString m_host;
  };

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

  UDPSocket::UDPSocket() : m_d(new D)
  {
    SocketWrapper::startup();
  }

  UDPSocket::UDPSocket(int fd) : m_d(new D(fd))
  {
    SocketWrapper::startup();
  }

  UDPSocket::~UDPSocket()
  {
    close();
    delete m_d;
  }

  int UDPSocket::openServer(int port, const char * bindAddress)
  {
    close();

    m_d->m_host.clear();
    m_d->m_port = port;

    QString errstr;
    int err = SocketUtilPosix::bindOrConnectSocket(m_d->m_fd, bindAddress, port, errstr,
                  true, AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if(err) {
      error("UDPSocket::open # Failed to bind to port %d: %s", port, errstr.toUtf8().data());
    }

    return err;
  }

  int UDPSocket::openClient(const char * host, int port)
  {
    close();

    m_d->m_host = host;
    m_d->m_port = port;

    QString errstr;
    int err = SocketUtilPosix::bindOrConnectSocket(m_d->m_fd, host, port, errstr,
                  false, AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if(err) {
      error("UDPSocket::openClient # Failed to connect %s:%d: %s", host, port, errstr.toUtf8().data());
    }

    return err;
  }

  bool UDPSocket::close()
  {
    int fd = m_d->m_fd;
    if(fd < 0)
      return false;

    m_d->m_fd = -1;

    if(!m_d->m_host.isEmpty() && shutdown(fd, SHUT_RDWR)) {
      error("UDPSocket::close # Failed to shut down the socket: %s", SocketWrapper::strerror(SocketWrapper::err()));
    }
    if(SocketWrapper::close(fd)) {
      error("UDPSocket::close # Failed to close socket: %s", SocketWrapper::strerror(SocketWrapper::err()));
    }

    return true;
  }

  bool UDPSocket::isPendingInput(unsigned int waitMicroSeconds)
  {
    if(m_d->m_fd < 0)
      return false;

    struct pollfd pfd;
    memset( & pfd, 0, sizeof(pfd));

    pfd.fd = m_d->m_fd;
    pfd.events = POLLRDNORM;
    int status = SocketWrapper::poll(&pfd, 1, waitMicroSeconds / 1000);
    if(status == -1) {
      Radiant::error("UDPSocket::isPendingInput %s", SocketWrapper::strerror(SocketWrapper::err()));
    }

    return (pfd.revents & POLLRDNORM) == POLLRDNORM;
  }

  bool UDPSocket::isOpen() const
  {
    return m_d->m_fd >= 0;
  }

  int UDPSocket::read(void * buffer, int bytes, bool waitfordata = false)
  {
    return read(buffer, bytes, waitfordata, false);
  }

  int UDPSocket::read(void * buffer, int bytes, bool waitfordata, bool readAll)
  {
    if(m_d->m_fd < 0 || bytes < 0)
      return -1;

    int pos = 0;
    char * data = reinterpret_cast<char*>(buffer);

#ifdef RADIANT_WINDOWS
    // Windows doesn't implement MSG_DONTWAIT, so do an extra poll
    if(!waitfordata && !readAll){
      struct pollfd pfd;
      pfd.fd = m_d->m_fd;
      pfd.events = POLLIN;
      if(SocketWrapper::poll(&pfd, 1, 0) <= 0 || (pfd.revents & POLLIN) == 0)
        return 0;
    }
    int flags = 0;
#else
    int flags = (readAll || waitfordata) ? 0 : MSG_DONTWAIT;
#endif

    while(pos < bytes) {
      SocketWrapper::clearErr();
      // int max = bytes - pos > SSIZE_MAX ? SSIZE_MAX : bytes - pos;
      int max = bytes - pos > 32767 ? 32767 : bytes - pos;
      /// @todo should we care about the sender?
      int tmp = recv(m_d->m_fd, data + pos, max, flags);

      if(tmp > 0) {
        pos += tmp;
        if(!readAll) return pos;
      } else if(tmp == 0 || m_d->m_fd == -1) {
        return pos;
      } else if(SocketWrapper::err() == EINTR) {
        continue;
      } else if(SocketWrapper::err() == EAGAIN || SocketWrapper::err() == EWOULDBLOCK) {
        if(readAll || (waitfordata && pos == 0)) {
          struct pollfd pfd;
          pfd.fd = m_d->m_fd;
          pfd.events = POLLIN;
          SocketWrapper::poll(&pfd, 1, 5000);
        } else {
          return pos;
        }
      } else {
        error("UDPSocket::read # Failed to read: %s", SocketWrapper::strerror(SocketWrapper::err()));
        return pos;
      }
    }

    return pos;
  }

  int UDPSocket::write(const void * buffer, int bytes)
  {
    if(m_d->m_fd < 0 || bytes < 0)
      return -1;

    if(m_d->m_host.isEmpty()) {
      /// @todo implement writeTo() or something similar
      error("UDPSocket::write # This socket was created using openServer, "
            "it's not connected. Use writeTo() instead.");
      return -1;
    }

    int pos = 0;
    const char * data = reinterpret_cast<const char*>(buffer);

    while(pos < bytes) {
      // int max = bytes - pos > SSIZE_MAX ? SSIZE_MAX : bytes - pos;
      int max = bytes - pos > 32767 ? 32767 : bytes - pos;
      int tmp = send(m_d->m_fd, data + pos, max, 0);
      if(tmp > 0) {
        pos += tmp;
      } else if(SocketWrapper::err() == EINTR) {
        continue;
      } else if(SocketWrapper::err() == EAGAIN || SocketWrapper::err() == EWOULDBLOCK) {
        struct pollfd pfd;
        pfd.fd = m_d->m_fd;
        pfd.events = POLLOUT;
        SocketWrapper::poll(&pfd, 1, 5000);
      } else {
        //error("UDPSocket::write # Failed to write: %s", SocketWrapper::strerror(SocketWrapper::err()));
        return pos;
      }
    }

    return pos;
  }

  bool UDPSocket::setReceiveBufferSize(size_t bytes)
  {
    if(m_d->m_fd < 0)
      return false;

    int n = static_cast<int> (bytes);

    if (setsockopt(m_d->m_fd, SOL_SOCKET, SO_RCVBUF, (const char*)&n, sizeof(n)) == -1) {
      return false;
    }
    return true;
  }

#ifdef RADIANT_LINUX
  TimeStamp UDPSocket::timestamp() const
  {
    if (m_d->m_fd < 0)
      return TimeStamp();

    struct timeval tv;
    if (ioctl(m_d->m_fd, SIOCGSTAMP, &tv) == -1)
      return TimeStamp();

    int64_t tmp = tv.tv_sec;
    tmp <<= 24;
    tmp |= (int64_t) (tv.tv_usec * (TimeStamp::FRACTIONS_PER_SECOND * 0.000001));
    return TimeStamp(tmp);
  }
#endif
}
