/* COPYRIGHT
 *
 * This file is part of Radiant.
 *
 * Copyright: MultiTouch Oy, Helsinki University of Technology and others.
 *
 * See file "Radiant.hpp" for authors and more details.
 *
 * This file is licensed under GNU Lesser General Public
 * License (LGPL), version 2.1. The LGPL conditions can be found in 
 * file "LGPL.txt" that is distributed with this source package or obtained 
 * from the GNU organization (www.gnu.org).
 * 
 */

#ifndef RADIANT_TCP_SOCKET_HPP
#define RADIANT_TCP_SOCKET_HPP

#include <Patterns/NotCopyable.hpp>

#include <Radiant/BinaryStream.hpp>

#include <string>

struct in_addr;

namespace Radiant {

  class Thread;

  /// A client TCP socket for connecting to remote hosts
  /** @author Tommi Ilmonen*/
  class RADIANT_API TCPSocket : public Radiant::BinaryStream, public Patterns::NotCopyable
  {
  public:
    TCPSocket();
    /// Construct a socket based on a file descriptor
    /** This method is potentially non-portable as not all platforms
        use file descriptors to handle sockets. */
    TCPSocket(int fd);
    ~TCPSocket();

    /** Turn the Nagle algorithm on or off. This controls the queuing of messages to fill packets.
      @param noDelay if true, the queing is not used and packets are sent immediately
    */
    bool setNoDelay(bool noDelay);

    /// Opens a TCP socket to desired host:port
    /** @return On successful execution, returns zero, otherwise an
        error code (as in errno.h). */
    int open(const char * host, int port);
    /// Closes the socket
    bool close();

    /// Returns true of the socket is open.
    bool isOpen() const;

    /// Returns the hostname
    const char * host() const;
    /// Returns the port number
    int port() const;

    /// Read bytes from the socket
    /** @param buffer pointer to a buffer to store the read data to
        @param bytes how many bytes the buffer has room for
        @param waitfordata Conditionally wait for all the data to arrive.
        @return the number of bytes actually read
        */
    int read(void * buffer, int bytes, bool waitfordata = true);
    /// Write bytes to the socket
    int write(const void * buffer, int bytes);
    
    /// Returns true if the socket has been closed
    bool isHungUp() const;

    /// Return 'true' if readable data is pending.
    bool isPendingInput(unsigned int waitMicroSeconds = 0);

    //void debug();

    /** Moves the ownership of the socket to another thread. Sockets can not be
    created and used in different threads unless the ownership is moved with
    this function. */
    void moveToThread(Thread * t);

  private:
    friend class TCPServerSocket;

    class D;
    D * m_d;
  };

}

#endif
