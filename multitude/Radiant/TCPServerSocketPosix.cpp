/* Copyright (C) 2007-2013: Multi Touch Oy, Helsinki University of Technology
 * and others.
 *
 * This file is licensed under GNU Lesser General Public License (LGPL),
 * version 2.1. The LGPL conditions can be found in file "LGPL.txt" that is
 * distributed with this source package or obtained from the GNU organization
 * (www.gnu.org).
 * 
 */

#include "TCPServerSocket.hpp"
#include "TCPSocket.hpp"
#include "SocketUtilPosix.hpp"
#include "SocketWrapper.hpp"
#include "Trace.hpp"

#include <sys/types.h>
#include <stdio.h>

namespace Radiant
{

  class TCPServerSocket::D {
    public:
      D() : m_fd(-1), m_port(0) {}

      int m_fd;
      int m_port;
      QString m_host;
  };

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

  TCPServerSocket::TCPServerSocket()
    : m_d(new D)
  {
    SocketWrapper::startup();
  }
  
  TCPServerSocket::~TCPServerSocket()
  {
    debug("TCPServerSocket::~TCPServerSocket");
    close();
    delete m_d;
  }

  const QString TCPServerSocket::host() const {
    return m_d->m_host;
  }

  int TCPServerSocket::port() const {
    return m_d->m_port;
  }


  int TCPServerSocket::open(const char * host, int port, int maxconnections)
  {
    close();

    m_d->m_host = host ? host : "";
    m_d->m_port = port;

    QString errstr;
    int fd = -1;
    int err = SocketUtilPosix::bindOrConnectSocket(fd, host, port, errstr,
                  true, AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if(err) {
      error("TCPServerSocket::open # %s", errstr.toUtf8().data());
      return err;
    }

    if(::listen(fd, maxconnections) != 0) {
      int err = SocketWrapper::err();
      error("TCPServerSocket::open # Failed to listen TCP socket: %s", SocketWrapper::strerror(err));
      SocketWrapper::close(fd);
      return err ? err : -1;
    }

    m_d->m_fd = fd;

    return 0;
  }

  bool TCPServerSocket::close()
  {
    int fd = m_d->m_fd;
    if(fd < 0)
      return false;

    m_d->m_fd = -1;
    m_d->m_host = "";
    m_d->m_port = 0;

    if(::shutdown(fd, SHUT_RDWR)) {
      debug("TCPServerSocket::close # Failed to shut down the socket: %s", SocketWrapper::strerror(SocketWrapper::err()));
    }

    if(SocketWrapper::close(fd)) {
      error("TCPServerSocket::close # Failed to close socket: %s", SocketWrapper::strerror(SocketWrapper::err()));
    }

    return true;
  }

  bool TCPServerSocket::isOpen() const
  {
    return m_d->m_fd >= 0;
  }

  bool TCPServerSocket::isPendingConnection(unsigned int waitMicroSeconds)
  {
    if(m_d->m_fd < 0)
      return false;

    struct pollfd pfd;
    memset( & pfd, 0, sizeof(pfd));
    pfd.fd = m_d->m_fd;
    pfd.events = POLLRDNORM;
    int status = SocketWrapper::poll(&pfd, 1, waitMicroSeconds / 1000);
    if(status == -1) {
      Radiant::error("TCPServerSocket::isPendingConnection %s", SocketWrapper::strerror(SocketWrapper::err()));
    }

    return (pfd.revents & POLLRDNORM) == POLLRDNORM;
  }

  TCPSocket * TCPServerSocket::accept()
  {
    if(m_d->m_fd < 0)
      return 0;

    sockaddr newAddress;
    socklen_t addressLength(sizeof(newAddress));

    memset( & newAddress, 0, sizeof(newAddress));

    for(;;) {
      SocketWrapper::clearErr();
      int fd = ::accept(m_d->m_fd, (sockaddr *) & newAddress, & addressLength);

      if(fd >= 0)
        return new TCPSocket(fd);

      if(fd < 0) {
        if(m_d->m_fd == -1)
          return 0;
        int err = SocketWrapper::err();
        if(err == EAGAIN || err == EWOULDBLOCK) {
          struct pollfd pfd;
          pfd.fd = m_d->m_fd;
          pfd.events = POLLIN;
          SocketWrapper::poll(&pfd, 1, 5000);
        } else {
          error("TCPServerSocket::accept # %s", SocketWrapper::strerror(SocketWrapper::err()));
          return 0;
        }
      }
    }

    return 0;
  }
}

