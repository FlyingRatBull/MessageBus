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

#include "localsocketprivate_unix_worker.h"

#include <sys/ioctl.h>
#include <sys/un.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <limits>
#include <cstdio>
#include <errno.h>
#include <poll.h>

#include "localsocket.h"

#define O_Q LocalSocket * const q = p->q_func()


LocalSocketPrivate_Worker::LocalSocketPrivate_Worker(LocalSocketPrivate * pp)
:	QObject(0), p(pp), run(true), m_readNotifier(pp->m_socket, QSocketNotifier::Read, this)
, m_writeNotifier(pp->m_socket, QSocketNotifier::Write, this)
, m_tmpData(pp->m_socketReadBufferSize, (char)0)
, m_writeBytesTillNewPackage(0)
, m_reveivingSocket(false)
{
	connect(&m_readNotifier, SIGNAL(activated(int)), SLOT(read()), Qt::DirectConnection);
	connect(&m_writeNotifier, SIGNAL(activated(int)), SLOT(write()));
	
	m_writeNotifier.setEnabled(false);
	
	INIT_HEADER(m_readHeader);
}


LocalSocketPrivate_Worker::~LocalSocketPrivate_Worker()
{
}


void LocalSocketPrivate_Worker::init()
{
	read();
	write();
}


void LocalSocketPrivate_Worker::read()
{
	// 			qDebug("read!");
	
	m_readNotifier.setEnabled(false);
	
	if(!run)
	{
		p->exception();
		return;
	}
	
	// 			qDebug("-> readImpl");
	bool	retry	=	readImpl();
	// 			qDebug("<- readImpl");
	
	if(!run)
	{
		p->exception();
		return;
	}
	
	if(retry)
		callSlotQueued(this, "read");
	else
		callSlotQueued(&m_readNotifier, "setEnabled", Q_ARG(bool, true));
	// 				m_readNotifier.setEnabled(true);
		
		// 			qDebug("~read!");
}


bool LocalSocketPrivate_Worker::readImpl()
{
	// 			qDebug("readImpl!");
	O_Q;
	
	#define SAFE_READ(_target, _size) int read=0;int tmp=::read(p->m_socket,_target,(_size));if(tmp>0)read=tmp;
	#define SAFE_READ_FIRST(_target, _size) int tmp=::read(p->m_socket,_target,(_size));if(tmp>0 && tmp<_size){m_readNotifier.setEnabled(false);p->exception();return false;}
	
	#define CHECK_TMP_READ(READ_MSG) if(tmp==0||errno==EBADF){m_readNotifier.setEnabled(false);p->exception();return false;}else if(tmp<0 && !(errno==EAGAIN || errno==EWOULDBLOCK)){perror("LocalSocket: Error when reading " READ_MSG ": ");p->exception();return false;}
	
	// Check whether to read header
	if(m_readHeader.cmd == 0)
	{
		// Only process when header can be read
		size_t	bytesAvailable	=	0;
		if(ioctl(p->m_socket, FIONREAD, (char*)&bytesAvailable) >= 0)
		{
			if(bytesAvailable < sizeof(m_readHeader))
			{
				// Disable socket if it cannot be written to
				struct	pollfd	pfd;
				pfd.fd	=	p->m_socket;
				pfd.events	=	POLLRDHUP;
				
				if(poll(&pfd, 1, 0) == 1)
					run	=	false;
				
				return false;
			}
		}
		
		SAFE_READ_FIRST((char*)&m_readHeader, sizeof(m_readHeader));
		// Check for errors and EOF
		CHECK_TMP_READ("header");
	}
	
	if(!m_readHeader.size || m_readHeader.size > p->m_socketReadBufferSize)
	{
		m_readNotifier.setEnabled(false);
		
		p->exception();
		return false;
	}
	
	// 			qDebug("Got size: %u", size);
	
	switch(m_readHeader.cmd)
	{
		case CMD_PKG:
		case CMD_DATA:
		{
			// 					qDebug("CMD_DATA (size: %u)", size);
			
			// Read data
			SAFE_READ(m_tmpData.data(), m_readHeader.size);
			// Check for errors and EOF
			CHECK_TMP_READ("data");
			
			// 					qDebug("[0x%08X]  ... read %d bytes (of %u) ([0]=%u)", (int)this, read, m_readHeader.size, (uchar)(m_tmpData.constData()[0]));
			
			if(read == 0)
				return false;
			
			if(m_readHeader.cmd == CMD_DATA)
			{
				// Queue data for reading
				p->readBuffer.enqueue(m_tmpData.constData(), read);
				
				// 						qDebug("readBuffer.size() = %d", p->readBuffer.size());
				
				emit(p->readyRead());
			}
			else if(m_readHeader.cmd == CMD_PKG)
			{
				// 						qDebug("Appending package - current size: %d; appending: %d", m_tmpPkg.size(), read);
				m_tmpPkg.append(m_tmpData.constData(), read);
			}
			
			// Check for partial read
			if(read < m_readHeader.size)
			{
				// 						qDebug("Reducing size");
				m_readHeader.size	-=	read;
				return true;		// Retry reading
			}
		}break;
		
		case CMD_PKG_END:
		{
			quint32	pkgSize;
			
			// Read package size
			SAFE_READ((char*)&pkgSize, sizeof(pkgSize));
			// Check for errors and EOF
			CHECK_TMP_READ("package size");
			
			// Check if last package is valid
			if(m_tmpPkg.size() != pkgSize)
			{
				qDebug("Expected size: %d; size: %d", pkgSize, m_tmpPkg.size());
				fprintf(stderr, "LocalSocket: Error when reading package!\n");
				
				p->exception();
				return false;
			}
			
			p->readPkgBuffer.enqueue(m_tmpPkg);
			m_tmpPkg.clear();
			
			emit(p->readyReadPackage());
		}break;
		
		case CMD_SOCK:
		{
			// Only read package if all previous sockets were completely read
			if(!m_reveivingSocket)
			{
				// Check if we can read the package and the "unneded" byte
				size_t	bytesAvailable	=	0;
				if(ioctl(p->m_socket, FIONREAD, (char*)&bytesAvailable) >= 0)
				{
					if(bytesAvailable < sizeof(sizeof(int) + 1 /* unneeded byte */))
						return false;	// Wait for another read event
				}
				
				// Read package
				int	socketDescriptor;
				SAFE_READ((char*)&socketDescriptor, sizeof(socketDescriptor));
				CHECK_TMP_READ("socket descriptor package");
				
				// Read actual socket descriptor
				bzero(&m_recMsg, sizeof(m_recMsg));
				bzero(&m_recIov, sizeof(m_recIov));
				
				char							buf[1];
				int								connfd	=	-1;
				char							ccmsg[CMSG_SPACE(sizeof(connfd))];
				
				m_recIov.iov_base	=	buf;
				m_recIov.iov_len		=	1;
				
				m_recMsg.msg_name		=	0;
				m_recMsg.msg_namelen	=	0;
				m_recMsg.msg_iov			=	&m_recIov;
				m_recMsg.msg_iovlen	=	1;
				/*
				 o ld BSD im*plementations should use msg_accrights instead of
				 msg_control; the interface is different.
				 */
				m_recMsg.msg_control			=	ccmsg;
				m_recMsg.msg_controllen	=	sizeof(ccmsg); /* ? seems to work... */
			}
			
			int	rv	=	recvmsg(p->m_socket, &m_recMsg, 0);
			if(rv == -1 && (errno==EAGAIN || errno==EWOULDBLOCK))
			{
				///@test Source of deadlock?
				///@todo Test how long this happens until it gets skipped
				m_reveivingSocket	=	true;
				return true;
			}
			else if(rv < 1)
			{
				perror("LocalSocket: Error when reading socket descriptor");
				
				p->exception();
				return false;
			}
			
			m_reveivingSocket	=	false;
			
			m_recCmsg = CMSG_FIRSTHDR(&m_recMsg);
			
			if(!m_recCmsg)
			{
				fprintf(stderr, "CTRL_SOCK seems to be broken in LocalSocketPrivate_unix!\n");
				break;
			}
			
			if(m_recCmsg->cmsg_type != SCM_RIGHTS)
			{
				fprintf(stderr, "LocalSocket: Got control message of unknown type %d\n",
								m_recCmsg->cmsg_type);
				break;
			}
			
			int	actualSocketDescriptor	=	*(int*)CMSG_DATA(m_recCmsg);
			
			p->readSDescBuffer.enqueue(actualSocketDescriptor);
			
			emit(p->readyReadSocketDescriptor());
		}break;
		
		default:
		{
			fprintf(stderr, "LocalSocket: Error reading command from socket (Unexpected command: 0x%08X)!\n", m_readHeader.cmd);
			
			p->exception();
			return false;
		}break;
	}
	
	// Invalidate header to reread at next call
	INIT_HEADER(m_readHeader);
	
	return false;
}


void LocalSocketPrivate_Worker::write()
{
	#define SAFE_WRITE(_source, _size) int written=0;int tmp=::write(p->m_socket,(const char*)(_source),(_size));if(tmp>0)written=tmp;
	
	#define CHECK_TMP_WRITE(WRITE_MSG) if(tmp==0||errno==EBADF){p->exception();return false;}else if(tmp<0 && !(errno==EAGAIN || errno==EWOULDBLOCK)){perror("LocalSocket: Error when writing " WRITE_MSG);p->exception();return false;}
	
	// 			qDebug("write!");
	
	m_writeNotifier.setEnabled(false);
	
	if(!run)
	{
		// 				qDebug("~write!");
		return;
	}
	
	bool	retry	=	writeSocketDescriptors();
	
	if(retry)
	{
		// 				qDebug("~write!");
		callSlotQueued(&m_writeNotifier, "setEnabled", Q_ARG(bool, true));
		// 				m_writeNotifier.setEnabled(true);
		return;
	}
	
	retry	=	writeData();
	
	if(retry)
	{
		// 				qDebug("~write!");
		callSlotQueued(&m_writeNotifier, "setEnabled", Q_ARG(bool, true));
		// 				m_writeNotifier.setEnabled(true);
		return;
	}
	
	// 			qDebug("~write!");
}


bool LocalSocketPrivate_Worker::writeData()
{
	if(p->writeBuffer.isEmpty())
		return false;
	
	// Only write rest of package if started writing to ensure writeSocketDescriptors() doesn't break packages
		QByteArray	data(p->writeBuffer.peek(m_writeBytesTillNewPackage ? m_writeBytesTillNewPackage : p->m_maxSendPackageSize));
		
		if(data.isEmpty())
			return true;
		
		// Get header
			if(!m_writeBytesTillNewPackage)
			{
				struct LocalSocketPrivate::Header	header;
				memcpy(&header, data.constData(), sizeof(header));
				
				m_writeBytesTillNewPackage	=	sizeof(header) + header.size;
			}
			
			O_Q;
			
			SAFE_WRITE(data.constData(), data.size());
			CHECK_TMP_WRITE("data");
			
			// 			qDebug("%d bytes written (of %d)", written, data.size());
			
			m_writeBytesTillNewPackage	-=	written;
			
			// Discard written data
			if(written && p->writeBuffer.discard(written) < written)
			{
				fprintf(stderr, "LocalSocket: Error when discarding data!\n");
				
				p->exception();
				return false;
			}
			
			if(!p->writeBuffer.isEmpty())
				callSlotQueued(&m_writeNotifier, "setEnabled", Q_ARG(bool, true));
			
			return false;
}


bool LocalSocketPrivate_Worker::writeSocketDescriptors()
{
	if(p->writeSDescBuffer.isEmpty())
		return false;
	
	int	socketDescriptor	=	0;
	
	struct LocalSocketPrivate::Header	header;
	INIT_HEADER(header);
	
	header.cmd	=	CMD_SOCK;
	header.size	=	sizeof(socketDescriptor);
	
	// Prepare to write socket descriptor
	
	struct	msghdr		msg;
	char							ccmsg[CMSG_SPACE(sizeof(socketDescriptor))];
	struct	cmsghdr	*	cmsg;
	struct	iovec			vec;	/* stupidity: must send/receive at least one byte */
	void						*	str	=	(void*)"x";
	int								rv;
	
	msg.msg_name		=	0;
	msg.msg_namelen	=	0;
	
	vec.iov_base		=	str;
	vec.iov_len			=	1;
	msg.msg_iov			=	&vec;
	msg.msg_iovlen	=	1;
	
	/*
	 o ld BSD implem*entations should use msg_accrights instead of
	 msg_control; the interface is different.
	 */
	msg.msg_control			=	ccmsg;
	msg.msg_controllen	=	sizeof(ccmsg);
	
	cmsg										=	CMSG_FIRSTHDR(&msg);
	cmsg->cmsg_level				=	SOL_SOCKET;
	cmsg->cmsg_type					=	SCM_RIGHTS;
	cmsg->cmsg_len					=	CMSG_LEN(sizeof(socketDescriptor));
	msg.msg_controllen			=	cmsg->cmsg_len;
	msg.msg_flags						=	0;
	
	// Send package
	QByteArray	pkg;
	pkg.append((const char*)&header, sizeof(header));
	pkg.append((const char*)&socketDescriptor, sizeof(socketDescriptor));
	
	// Check if write buffer has enough space
	// 			size_t	bytesWritable	=	0;
	// 			if(ioctl(p->m_socket, FIONWRITE, (char*)&bytesWritable) >= 0)
	// 			{
		// 				bytesWritable	=	p->m_maxSendPackageSize - bytesWritable;
	// 				
	// 				if(bytesWritable < (sizeof(header) + header.size + sizeof(msg) + sizeof(ccmsg) + sizeof(vec) + 1))
	// 					return true;
	// 			}
	
	// Set actual socket descriptor
	socketDescriptor	=	p->writeSDescBuffer.dequeue();
	*(int*)CMSG_DATA(cmsg)	=	socketDescriptor;
	
	// Set to blocking for writing socket descriptor
	int	flags	=	fcntl(p->m_socket, F_GETFL, 0);
	fcntl(p->m_socket, F_SETFL, flags & ~O_NONBLOCK);
	
	SAFE_WRITE(pkg.constData(), pkg.size());
	CHECK_TMP_WRITE("socket descriptor package");
	
	// Send socket descriptor
	rv	=	sendmsg(p->m_socket, &msg, 0);
	
	// Reset to previous flags
	fcntl(p->m_socket, F_SETFL, flags);
	
	if(rv < 1)
	{
		fprintf(stderr, "LocalSocket: Error when writing socket descriptor!");
		
		p->exception();
		return true;
	}
	else
		return false;
}
