/*
 *  MessageBus - Inter process communication library
 *  Copyright (C) 2013  Oliver Becker <der.ole.becker@gmail.com>
 * 
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 * 
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 * 
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "localsocketprivate_unix.h"

#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/un.h>
#include <unistd.h>
#include <fcntl.h>
#ifdef USE_SELECT
#include <select.h>
#endif
#include <errno.h>
#include <malloc.h>

LocalSocketPrivate_Unix::LocalSocketPrivate_Unix(LocalSocket* q)
	:	LocalSocketPrivate(q), m_readBufferSize(0)
#ifndef USE_SELECT
  , m_epollFd_rw(0), m_epollFd_r(0), m_epollFd_w(0)
#endif
{
	// Control message needs only space for ine file descriptor
	m_ccmsgSize	=	CMSG_SPACE(sizeof(quintptr));
	m_ccmsg			=	(char*)malloc(m_ccmsgSize);
}


LocalSocketPrivate_Unix::~LocalSocketPrivate_Unix()
{
	free(m_ccmsg);
}


bool LocalSocketPrivate_Unix::connectToServer(const QString& filename)
{
// 	qDebug("[%p] LocalSocketPrivate_Unix::connectToServer()", this);
	
	// Try to open socket
	quintptr	socketDescriptor	=	::socket(AF_UNIX, SOCK_STREAM, 0);
	
	if(socketDescriptor < 1)
	{
    setError(QString("Cannot open socket: %1").arg(strerror(errno)));
		return false;
	}
	
	struct	sockaddr_un	serv_addr;
	serv_addr.sun_family	=	AF_UNIX;
	QByteArray	fn(filename.toLocal8Bit());
	memcpy(serv_addr.sun_path, fn.constData(), fn.length() + 1);
	
	if(::connect(socketDescriptor, (sockaddr*)&serv_addr, sizeof(serv_addr)) < 0)
	{
		setError(QString("Cannot connect to server: %1").arg(strerror(errno)));
		return false;
	}
	
	return setSocketDescriptor(socketDescriptor);
}


bool LocalSocketPrivate_Unix::setSocketDescriptor(quintptr socketDescriptor)
{
	// Set to non blocking
// 	int	flags	=	fcntl(socketDescriptor, F_GETFL, 0);
// 	fcntl(socketDescriptor, F_SETFL, flags | O_NONBLOCK);
	
	// Don't emit SIGPIPE signal but return EPIPE on systems that support it
#ifdef SO_NOSIGPIPE
	int set = 1;
	setsockopt(socketDescriptor, SOL_SOCKET, SO_NOSIGPIPE, (void *)&set, sizeof(int));
#endif
	
	m_socketDescriptor	=	socketDescriptor;
	
	// Get read buffer size
	int				buff;
	socklen_t	optlen	=	sizeof(buff);
	int	tmp	=	getsockopt(m_socketDescriptor, SOL_SOCKET, SO_RCVBUF, &buff, &optlen);
	
	if(tmp > 0)
		m_readBufferSize	=	tmp - (sizeof(msghdr) + sizeof(iovec) + sizeof(cmsghdr));
	else
		m_readBufferSize	=	LocalSocketPrivate::readBufferSize();
  
#ifndef USE_SELECT
  // Init epoll()
  if(m_epollFd_rw <= 0) {
    m_epollFd_rw  = epoll_create1(0);
    m_epollFd_r   = epoll_create1(0);
    m_epollFd_w   = epoll_create1(0);
  }
  
  m_epollEvent_rw.events  = EPOLLIN | EPOLLOUT;
  m_epollEvent_r.events   = EPOLLIN;
  m_epollEvent_w.events   = EPOLLOUT;
  
  // Add watched fd
  if(epoll_ctl(m_epollFd_rw, EPOLL_CTL_ADD, m_socketDescriptor, &m_epollEvent_rw) != 0
    || epoll_ctl(m_epollFd_r, EPOLL_CTL_ADD, m_socketDescriptor, &m_epollEvent_r) != 0
    || epoll_ctl(m_epollFd_w, EPOLL_CTL_ADD, m_socketDescriptor, &m_epollEvent_w) != 0
  ) {
    close();
    setError(QString("Cannot set socket descriptor: %1").arg(strerror(errno)));
    return false;
  }
  
#endif

	setOpened();
	
	return true;
}


void LocalSocketPrivate_Unix::close()
{
	if(m_socketDescriptor && ::close(m_socketDescriptor) != 0)
	{
		m_socketDescriptor	=	0;
    setError(QString("Could not close socket correctly: %1").arg(strerror(errno)));
	}

	setClosed();
}


int LocalSocketPrivate_Unix::availableWriteBufferSpace() const
{
	if(!m_socketDescriptor)
		return -1;
	
	// Get send buffer size
	int				buff;
	socklen_t	optlen	=	sizeof(buff);
	int	tmp	=	getsockopt(m_socketDescriptor, SOL_SOCKET, SO_SNDBUF, &buff, &optlen);
	
	if(tmp > 0)
		return tmp;
	else
		return LocalSocketPrivate::availableWriteBufferSpace();
}


int LocalSocketPrivate_Unix::readBufferSize() const
{
	return m_readBufferSize;
}


int LocalSocketPrivate_Unix::write(const char* data, int size, quintptr* fileDescriptor)
{
// 	qDebug("[%p] LocalSocketPrivate_Unix::write()", this);
	
	if(!m_socketDescriptor)
		return 0;
	
	bzero(&m_msgHeader, sizeof(m_msgHeader));
	bzero(&m_iovecData, sizeof(m_iovecData));

	// Set data
	m_iovecData.iov_base	=	(char*)data;
	m_iovecData.iov_len		=	size;		// Size of data in iov_base
	
	// Set message header
	m_msgHeader.msg_iov					=	&m_iovecData;
	m_msgHeader.msg_iovlen			=	1;	// Number of iov's in msg_iov
	m_msgHeader.msg_flags				=	0;
	
	// We need a control message if we send a file descriptor
	if(fileDescriptor)
	{
		bzero(m_ccmsg, m_ccmsgSize);
		
		// Add control message to message header
		m_msgHeader.msg_control			=	m_ccmsg;
		m_msgHeader.msg_controllen	=	m_ccmsgSize;
		
		struct	cmsghdr	*	cmsg	=	CMSG_FIRSTHDR(&m_msgHeader);
		cmsg->cmsg_len					=	CMSG_LEN(sizeof(quintptr));
		cmsg->cmsg_level				=	SOL_SOCKET;
		cmsg->cmsg_type					=	SCM_RIGHTS;
		
		// Set the actual file descriptor
		*(int*)CMSG_DATA(cmsg)	=	(*fileDescriptor);
	}

	// Send data
// 	qDebug("Sending: %s...", QByteArray(data).left(10).toHex().constData());
	int	result	=	sendmsg(m_socketDescriptor, &m_msgHeader, MSG_NOSIGNAL);

	if(result < 1)
	{
		// The socket is closed
// 		if(errno == ENOTCONN || errno == ECONNRESET || ENOTSOCK)
// 		{
// 			if(errno == ECONNRESET)
// 				setError("Connection reset by peer!");
// 			else
// 				setClosed();
// 		}
// 		else
			setError(QString("Could not write data: %1").arg(strerror(errno)));

		return 0;
	}
	else
		return result;
}


int LocalSocketPrivate_Unix::read(char* data, int size)
{
// 	qDebug("[%p] LocalSocketPrivate_Unix::read()", this);
	
	if(!m_socketDescriptor)
	{
// 		qDebug("  !m_socketDescriptor");
		return -1;
	}
	
	// Message header must have enough space in iov to store the data
	struct	cmsghdr	*	cmsg	=	0;
	
	bzero(&m_msgHeader, sizeof(m_msgHeader));
	bzero(&m_iovecData, sizeof(m_iovecData));

	// Set data buffer
	m_iovecData.iov_base	=	data;
	m_iovecData.iov_len		=	size;		// Size of data in iov_base
	
	// Set message header
	m_msgHeader.msg_iov					=	&m_iovecData;
	m_msgHeader.msg_iovlen			=	1;	// Number of iov's in msg_iov
	m_msgHeader.msg_control			=	m_ccmsg;
	m_msgHeader.msg_controllen	=	m_ccmsgSize;
	
	// Set to non blocking for reading
	int	flags	=	fcntl(m_socketDescriptor, F_GETFL, 0);
	fcntl(m_socketDescriptor, F_SETFL, flags | O_NONBLOCK);
	
	ssize_t	readBytes	=	::recvmsg(m_socketDescriptor, &m_msgHeader, 0);
	
// 	qDebug("  readBytes = %ld", readBytes);
	
	// Reset to previous flags
	fcntl(m_socketDescriptor, F_SETFL, flags);
	
	// An error occoured
	if(readBytes < 0)
	{
		// Check errors
		if(errno!=EAGAIN && errno!=EWOULDBLOCK)
			setError(QString("Could not read data: %1").arg(QString::fromLocal8Bit(strerror(errno))));
		
		return 0;
	}
	
	// No data was available
	if(readBytes == 0)
	{
		// Check if socket is really open
		// QSocketNotifier fires infinitely if reactivated and socket is closed
		int	numWritten	=	::send(m_socketDescriptor, 0, 0, MSG_NOSIGNAL);

		// We couldn't write data => Socket is closed
		if(numWritten < 0)
		{
			setError(QString("Could not read data: %1").arg(QString::fromLocal8Bit(strerror(errno))));
			return 0;
		}
		
		return 0;
	}
	
	// Message successfully read
	cmsg	=	CMSG_FIRSTHDR(&m_msgHeader);
	
	// Handle incoming file descriptor
	if(cmsg)
	{
		// We currently only support SOL_SOCKET + SCM_RIGHTS
		if(cmsg->cmsg_level != SOL_SOCKET || cmsg->cmsg_type != SCM_RIGHTS)
		{
			qDebug("Wrong type!");
			return readBytes;
		}
		
		quintptr	readFileDescriptor	=	*(quintptr*)CMSG_DATA(cmsg);
		
		addReadFileDescriptor(readFileDescriptor);
	}
	
	return readBytes;
}


bool LocalSocketPrivate_Unix::waitForReadOrWrite(bool& readyRead, bool& readyWrite, int timeout)
{
// 	qDebug("[%p] LocalSocketPrivate_Unix::waitForReadOrWrite()", this);

  // select() is O(n) and has a very limited number of watched fds but is available almost everywhere
#ifdef USE_SELECT
  m_timeout.tv_sec  = (timeout/1000);
  m_timeout.tv_usec = (timeout%1000)*1000;
  
	FD_ZERO(&m_readFds);
	FD_ZERO(&m_writeFds);
	FD_ZERO(&m_excFds);
	
	if(readyRead)
		FD_SET(m_socketDescriptor, &m_readFds);
	if(readyWrite)
		FD_SET(m_socketDescriptor, &m_writeFds);
	FD_SET(m_socketDescriptor, &m_excFds);
	
  readyRead   = false;
  readyWrite  = false;

	int	retval	=	select(m_socketDescriptor+1, &m_readFds, &m_writeFds, &m_excFds, &m_timeout); 
	
	// Error (-1) or timeout (0)
	if(retval <= 0)
		return false;
	
// 	qDebug("  retval: %d (%s; %s; %s)", retval,
// 				FD_ISSET(m_socketDescriptor, &m_readFds) ? "true" : "false",
// 				FD_ISSET(m_socketDescriptor, &m_writeFds) ? "true" : "false",
// 				FD_ISSET(m_socketDescriptor, &m_excFds) ? "true" : "false"
// 	);
	
	if(FD_ISSET(m_socketDescriptor, &m_readFds))
		readyRead	=	true;
	if(FD_ISSET(m_socketDescriptor, &m_writeFds))
		readyWrite	=	true;
	
	if(FD_ISSET(m_socketDescriptor, &m_excFds))
	{
		setError("Exception in select()");
		return false;
	}
#else // epoll() is O(1) and very scalable for high number of fds but is only available since 2.5.44
  int nfds  = 0;
  
  // Default epoll behaviour is level triggerd so everything is ok
  if(readyRead && readyWrite)
    nfds  = epoll_wait(m_epollFd_rw, m_epollEvents, MAX_EPOLL_EVENTS, timeout);
  else if(readyRead)
    nfds  = epoll_wait(m_epollFd_r, m_epollEvents, MAX_EPOLL_EVENTS, timeout);
  else if(readyWrite)
    nfds  = epoll_wait(m_epollFd_w, m_epollEvents, MAX_EPOLL_EVENTS, timeout);

  readyRead   = false;
  readyWrite  = false;

  if(nfds < 0) {
    setError("Exception in epoll()");
    return false;
  }
  
  // Timeout
  if(nfds == 0)
    return true;
  
  for(int i = 0; i < nfds; i++) {
    readyRead   |= ((m_epollEvents[i].events & EPOLLIN) != 0);
    readyWrite  |= ((m_epollEvents[i].events & EPOLLOUT) != 0);
    
    if((m_epollEvents[i].events & (EPOLLERR | EPOLLHUP)) != 0) {
      setError("Exception in epoll()");
      return false;
    }
  }
#endif
	
	return true;
}
