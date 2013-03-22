/*
 *  MessageBus - Inter process communication library
 *  Copyright (C) 2012  Oliver Becker <der.ole.becker@gmail.com>
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

#include <QtCore/QString>

#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/un.h>
#include <unistd.h>
#include <fcntl.h>
#include <poll.h>
#include <errno.h>

LocalSocketPrivate_Unix::LocalSocketPrivate_Unix(LocalSocket *q)
:	LocalSocketPrivate(q), m_socket(0), m_readNotifier(0), m_doReadSocketDescriptor(false)
{
	///@todo Replace by setReadBufferSize() call
	m_readBuffer	=	QByteArray(8192, (char)0);
}


LocalSocketPrivate_Unix::~LocalSocketPrivate_Unix()
{
	m_readBuffer.clear();
	close();
}


quintptr LocalSocketPrivate_Unix::socketDescriptor() const
{
	return (quintptr)m_socket;
}


void LocalSocketPrivate_Unix::open(const QString &name, QIODevice::OpenMode mode)
{
	// Check if we are already open
	if(m_socket)
	{
		setError(LocalSocket::SocketAccessError, "Socket already open!");
		return;
	}
	
	// Try to open socket
	m_socket	=	::socket(AF_UNIX, SOCK_STREAM, 0);
	
	if(m_socket < 1)
	{
		///@todo Analyze errno
		setError(LocalSocket::SocketAccessError, "Cannot open socket!");
		return;
	}
	
	struct	sockaddr_un	serv_addr;
	serv_addr.sun_family	=	AF_UNIX;
	QByteArray	fn(name.toLocal8Bit());
	memcpy(serv_addr.sun_path, fn.constData(), fn.length() + 1);
	
	if(::connect(m_socket, (sockaddr*)&serv_addr, sizeof(serv_addr)) < 0)
	{
		///@todo Analyze errno
		setError(LocalSocket::SocketAccessError, "Cannot connect to server!");
		return;
	}
	
// 	Logger::log("Bytes written (LocalSocketPrivateUnix)", 0, "O1", "Opened");
	
	// We now have an open socket
	open(m_socket, true, mode);
}


void LocalSocketPrivate_Unix::open(quintptr socketDescriptor, bool socketOpen, QIODevice::OpenMode mode)
{
	// Check if we are already open
	if(m_socket && m_socket != socketDescriptor)
	{
		setError(LocalSocket::SocketAccessError, "Socket already open!");
		return;
	}
	
	m_socket	=	socketDescriptor;
	
	// Set to non blocking
	int	flags	=	fcntl(m_socket, F_GETFL, 0);
	fcntl(m_socket, F_SETFL, flags | O_NONBLOCK);
	
	// Don't emit SIGPIPE signal but return EPIPE on systems that support it
#ifdef SO_NOSIGPIPE
	int set = 1;
	setsockopt(m_socket, SOL_SOCKET, SO_NOSIGPIPE, (void *)&set, sizeof(int));
#endif
	
	// Get send buffer size
	int				buff;
	socklen_t	optlen	=	sizeof(buff);
	int	tmp	=	getsockopt(m_socket, SOL_SOCKET, SO_SNDBUF, &buff, &optlen);
	
	// We received the send buffer size -> set it
	if(tmp > 0)
		setWriteBufferSize(tmp);
	
	// Create read notifier if we can read
	if(mode | QIODevice::ReadOnly)
	{
		m_readNotifier		=	new QSocketNotifier(m_socket, QSocketNotifier::Read, this);
		connect(m_readNotifier, SIGNAL(activated(int)), SLOT(readData()));
	}
	
	setOpened();
	
// 	Logger::log("Bytes written (LocalSocketPrivateUnix)", 0, "O2", "Opened");
}


void LocalSocketPrivate_Unix::close()
{
	// Delete read notifier
	if(m_readNotifier)
	{
		m_readNotifier->setEnabled(false);
		m_readNotifier->deleteLater();
		m_readNotifier	=	0;
	}
	
// 	qDebug("LocalSocketPrivate_Unix::close()");
	
	if(m_socket && ::close(m_socket) != 0)
	{
		m_socket	=	0;
		///@todo Analyze errno
		setError(LocalSocket::SocketAccessError, "Could not close socket!");
	}
	
	m_socket	=	0;
	
	setClosed();
	
// 	Logger::log("Bytes written (LocalSocketPrivateUnix)", 0, "C", "Closed");
}


void LocalSocketPrivate_Unix::readData()
{
	if(!isOpen())
		return;
	
// 	qDebug("readData() - open");
	
	// Read socket descriptor?
	if(m_doReadSocketDescriptor)
	{
		if(!readSocketDescriptor())
		{
			if(m_readNotifier && isOpen())
				m_readNotifier->setEnabled(true);
			// Socket descriptor could not be read -> retry next time
			return;
		}
	}
	// There must be at least one normal data portion (E.g. the next description of tan socket descriptor)
	// following an socket descriptor so we can safely asume that we recevie normal data at this point
	
	// Check if we're still open (readSocketDescriptor() can set errors)
	if(!isOpen())
		return;
	
// 	qDebug("readData() - still open");
	if(m_readNotifier && isOpen())
		m_readNotifier->setEnabled(false);
// 	qDebug("??? Reading data...");
		
	// Maximum bytes to read (Needed for usage of recvmsg() at the correct time)
	quint32	toRead	=	remainingReadBytes();
	
	if(toRead == 0)
		toRead	=	(quint32)m_readBuffer.size();
	
	// Read at most the size of our buffer
	toRead	=	qMin(toRead, (quint32)m_readBuffer.size());
	
	// Read data non blocking
	ssize_t	numRead	=	::recv(m_socket, m_readBuffer.data(), (size_t)toRead, MSG_NOSIGNAL);
	
// 	qDebug("Read %d bytes from socket", numRead);
	
	// We didn't get data
	if(numRead < 1)
	{
		// Read would have blocked
		if(errno == EAGAIN || errno == EWOULDBLOCK)
		{
			// Check if socket is really open
			// QSocketNotifier fires infinitely if reactivated and socket is closed
			int	numWritten	=	::send(m_socket, 0, 0, MSG_NOSIGNAL);

			// We couldn't write data => Socket is closed
			if(numWritten < 0)
			{
				close();
				return;
			}
			
			if(m_readNotifier && isOpen())
				m_readNotifier->setEnabled(true);
			return;
		}
		else
		{
			// recv only returns -1 when no data was available
			
			// The socket is closed
			if(errno == ENOTCONN || errno == ECONNRESET)
				close();
			else
				setError(LocalSocket::SocketAccessError, QString("Could not read from socket: %1").arg(strerror(errno)));
			
			return;
		}
	}
	
// 	qDebug("/*???*/ ok");
	
	// For debugging
	
	// Get number of available bytes
// 	size_t		bytesAvailable	=	0;
// 	if(ioctl(m_socket, FIONREAD, &bytesAvailable) < 0)
// 		qDebug("Could not get remaining bytes!");
	
// 	static quint64	totalRead	=	0;
// 	totalRead	+=	numRead;
// 	Logger::log("Bytes read (LocalSocketPrivateUnix)", totalRead);
// 	qDebug("total-read: %llu (remaining: %d)", totalRead, int(bytesAvailable));
	
	// Here we actually got data
	addReadData(m_readBuffer.constData(), numRead);
	
	// We successfully read data -> try again
	// Must be called queued to prevent SIGSEGV inside Qt for some reason
	callSlotQueued(this, "readData");
// 	m_readNotifier->setEnabled(true);
	
// 	qDebug("Got data!");
}


void LocalSocketPrivate_Unix::notifyReadSocketDescriptor()
{
	// Read socket descriptor upon next data arrival
	m_doReadSocketDescriptor	=	true;
}


bool LocalSocketPrivate_Unix::readSocketDescriptor()
{
// 	qDebug("??? Reading socket descriptor ...");
	// At this point we received an data portion which states that an socket descriptor is following. So we can read it now
	
	// Read actual socket descriptor
	bzero(&m_recMsg, sizeof(m_recMsg));
	bzero(&m_recIov, sizeof(m_recIov));
	
	char							buf[sizeof(int)];
	int								connfd	=	-1;
	char							ccmsg[CMSG_SPACE(sizeof(connfd))];
	
	m_recIov.iov_base					=	buf;
	m_recIov.iov_len					=	sizeof(quintptr);
	m_recMsg.msg_name					=	0;
	m_recMsg.msg_namelen			=	0;
	m_recMsg.msg_iov					=	&m_recIov;
	m_recMsg.msg_iovlen				=	1;
	m_recMsg.msg_control			=	ccmsg;
	m_recMsg.msg_controllen		=	sizeof(ccmsg);
	
	// Set to blocking for reading socket descriptor
	int	flags	=	fcntl(m_socket, F_GETFL, 0);
	///@todo Check if there is an MSG_ flag that we can pass, so we don't need to set to blocking mode for the whole socket each time
	fcntl(m_socket, F_SETFL, flags & ~O_NONBLOCK);
	
	int	rv	=	recvmsg(m_socket, &m_recMsg, 0);
	
	// Reset to previous flags
	fcntl(m_socket, F_SETFL, flags);
	
	// We didn't receive anything
	if(rv < 1)
	{
		if(errno==EAGAIN || errno==EWOULDBLOCK)
		{
			// We didn't get any data yet. Reread at next read attempt
			return false;
		}
		else
		{
// 			qDebug("!!! Cannot read socket descriptor");
			setError(LocalSocket::SocketAccessError, "Cannot read socket descriptor from socket!");
			return true;
		}
	}
	
// 	qDebug("??? ok");
	
	// True should be returned from this point on no matter if error or not
	// because false would cause an reread attempt
	
	// We got the socket -> reset state
	m_doReadSocketDescriptor	=	false;
	
// 	// Truncated message (test)
// 	if((m_recMsg.msg_flags & MSG_TRUNC) || (m_recMsg.msg_flags & MSG_CTRUNC))
// 	{
// 		qDebug("!!! CMSG_FIRSTHDR trunacted");
// 	}
	
	m_recCmsg = CMSG_FIRSTHDR(&m_recMsg);
	
	if(!m_recCmsg)
	{
// 		qDebug("!!! CMSG_FIRSTHDR failed (msg_controllen: %d)", m_recMsg.msg_controllen);
		setError(LocalSocket::SocketAccessError, "Cannot read socket descriptor header from socket!");
		return true;
	}
	
	if(m_recCmsg->cmsg_level != SOL_SOCKET)
	{
// 		qDebug("!!! SOL_SOCKET failed: %x", m_recCmsg->cmsg_type);
		setError(LocalSocket::SocketAccessError, QString("Got control message of unknown level %1 when trying to read an socket descriptor!").arg(m_recCmsg->cmsg_level));
		return true;
	}
	
	if(m_recCmsg->cmsg_type != SCM_RIGHTS)
	{
// 		qDebug("!!! SCM_RIGHTS failed: %x", m_recCmsg->cmsg_type);
		setError(LocalSocket::SocketAccessError, QString("Got control message of unknown type %1 when trying to read an socket descriptor!").arg(m_recCmsg->cmsg_type));
		return true;
	}
	
	quintptr	readSocketDescriptor	=	(quintptr)(*(int*)CMSG_DATA(m_recCmsg));
	quintptr	peerSocketDescriptor	=	*((quintptr*)buf);
	
// 	qDebug("=== %x  -  %x", readSocketDescriptor, peerSocketDescriptor);
	
	addReadSocketDescriptor(readSocketDescriptor, peerSocketDescriptor);
	
	return true;
}


bool LocalSocketPrivate_Unix::writeSocketDescriptor(quintptr socketDescriptor)
{
	if(!isOpen())
		return false;
	
	struct	msghdr		msgHeader;
	char							ccmsg[CMSG_SPACE(sizeof(socketDescriptor))];
	struct	iovec			data;	/* stupidity: must send/receive at least one byte */
	void						*	str	=	(void*)&socketDescriptor;
	int								rv;
	
	data.iov_base			=	str;
	data.iov_len			=	sizeof(int);		// Size of data in iov_base
	
	msgHeader.msg_name				=	0;
	msgHeader.msg_namelen			=	0;
	msgHeader.msg_iov					=	&data;
	msgHeader.msg_iovlen			=	1;	// Number of iov's in msg_iov
	msgHeader.msg_controllen	=	CMSG_LEN(sizeof(int));
	msgHeader.msg_flags				=	0;
	msgHeader.msg_control			=	ccmsg;
	msgHeader.msg_controllen	=	sizeof(ccmsg);
	
	struct	cmsghdr	*	cmsg	=	CMSG_FIRSTHDR(&msgHeader);
	cmsg->cmsg_len					=	CMSG_LEN(sizeof(int));
	cmsg->cmsg_level				=	SOL_SOCKET;
	cmsg->cmsg_type					=	SCM_RIGHTS;
	
	// Set the actual socket descriptor
	*(int*)CMSG_DATA(cmsg)	=	socketDescriptor;
	
	// Set to blocking for writing socket descriptor
	int	flags	=	fcntl(m_socket, F_GETFL, 0);
	///@todo Check if there is an MSG_ flag that we can pass, so we don't need to set to blocking mode for the whole socket each time
	fcntl(m_socket, F_SETFL, flags & ~O_NONBLOCK);
	
	// Send socket descriptor
// 	qDebug("writeSocketDescriptor");
	rv	=	sendmsg(m_socket, &msgHeader, MSG_NOSIGNAL);
	
	// Reset to previous flags
	fcntl(m_socket, F_SETFL, flags);
	
	if(rv < 1)
	{
		setError(LocalSocket::SocketAccessError, "Cannot write socket descriptor to socket!");
		return false;
	}
	else
		return true;
}


quint32 LocalSocketPrivate_Unix::writeData(const char *data, quint32 size)
{
// 	qDebug("LocalSocketPrivate_Unix::writeData");
	
	if(!isOpen())
		return 0;
	
	// Write data non blocking
	int	numWritten	=	::send(m_socket, data, size, MSG_NOSIGNAL);
	
	// We couldn't write data
	if(numWritten < 1)
	{
		// Write would have blocked
		if(errno == EAGAIN || errno == EWOULDBLOCK)
		{
			///@todo Use write notifier
			return 0;
		}
		else
		{
			setError(LocalSocket::SocketAccessError, QString("Could not write to socket: %1").arg(strerror(errno)));
			return 0;
		}
	}
	
	// For debugging
	
// 	static	quint64	totalWrittenLSPU	=	0;
// 	totalWrittenLSPU	+=	numWritten;
// 	qDebug("total-written (lspu): %llu", totalWrittenLSPU);
// 	Logger::log("Bytes written (LocalSocketPrivateUnix)", totalWrittenLSPU);
	
	return (quint32)numWritten;
}


void LocalSocketPrivate_Unix::flush()
{
	///@todo Implement flush for unix implementation
}
