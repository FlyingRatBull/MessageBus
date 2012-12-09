// Copyright 2012 Oliver Becker <der.ole.becker@gmail.com>
// 
// This file is part of the MessageBus project,
// 
// MessageBus is free software: you can redistribute it and/or modify it
// under the terms of the GNU General Public License as published by the Free Software Foundation,
// either version 3 of the License, or (at your option) any later version.
// MessageBus is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
// without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
// See the GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License along with MessageBus.
// If not, see http://www.gnu.org/licenses/.

#include "localsocketprivate_unix.h"

#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/un.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <limits>
#include <cstdio>
#include <errno.h>
#include <poll.h>

#include "localsocketprivate_unix_worker.h"

LocalSocketPrivate::LocalSocketPrivate(LocalSocket * q)
	:	q_ptr(q), w(0), m_socket(0)
{
	setObjectName("Thread: LocalSocketPrivate");
	
// 	writeBuffer.setUseGlobal(true);
// 	readBuffer.setUseGlobal(true);
}


LocalSocketPrivate::~LocalSocketPrivate()
{
	// Stop thread
	if(QThread::isRunning())
	{
		QThread::terminate();
		QThread::wait();
	}
}


void LocalSocketPrivate::setSocketFilename(const QString& socketFilename)
{
	m_socketFilename	=	socketFilename;
}


void LocalSocketPrivate::setSocketDescriptor(int socketDescriptor)
{
	m_socket	=	socketDescriptor;
}


void LocalSocketPrivate::notifyWrite()
{
	QTimer::singleShot(0, w, SLOT(write()));
}


bool LocalSocketPrivate::open(QIODevice::OpenMode mode)
{
	if(m_socket < 1)
	{
		if(m_socketFilename.isEmpty())
			return false;
		
		m_socket	=	::socket(AF_UNIX, SOCK_STREAM, 0);

		if(m_socket < 1)
		{
			perror("LocalSocket: Cannot open socket!");
			return false;
		}

		struct	sockaddr_un	serv_addr;
		serv_addr.sun_family	=	AF_UNIX;
		QByteArray	fn(m_socketFilename.toLocal8Bit());
		memcpy(serv_addr.sun_path, fn.constData(), fn.length() + 1);

		if(::connect(m_socket, (sockaddr*)&serv_addr, sizeof(serv_addr)) < 0)
		{
			perror("LocalSocket: Cannot connect to server!");
			return false;
		}
	}
	
	// Set to non blocking
	int	flags	=	fcntl(m_socket, F_GETFL, 0);
	fcntl(m_socket, F_SETFL, flags | O_NONBLOCK);
	
	// Get send buffer size
	int				buff;
	socklen_t	optlen	=	sizeof(buff);
	int	tmp	=	getsockopt(m_socket, SOL_SOCKET, SO_SNDBUF, &buff, &optlen);
	
	if(tmp < 0)
	{
		fprintf(stderr, "LocalSocket: Cannot retreive socket send buffer size!");
		Q_Q(LocalSocket);
		q->close();
		return false;
	}
	
	m_maxSendPackageSize	=	buff;
	
	// Get read buffer size
	tmp	=	getsockopt(m_socket, SOL_SOCKET, SO_RCVBUF, &buff, &optlen);
	
	if(tmp < 0)
	{
		fprintf(stderr, "LocalSocket: Cannot retreive socket send buffer size!");
		Q_Q(LocalSocket);
		q->close();
		return false;
	}
	
	// Max package size is max read buffer size excl. header size
	m_socketReadBufferSize	=	buff - sizeof(Header);

	// Start thread for reading/writing
	start();
	
	return true;
}


void LocalSocketPrivate::close()
{
// 	qDebug("LocalSocketPrivate::close()");
	
	QThread::exit(0);
	
	if(currentThread() != this)
		QThread::wait();
}


void LocalSocketPrivate::writeData(const QByteArray& package)
{
	Q_Q(LocalSocket);
	
	if(!m_socket)
	{
		q->close();
		return;
	}
	
	if(package.isEmpty())
		return;
	
	QByteArray	data;
	int					pos	=	0;
	struct	Header	header;
	INIT_HEADER(header);
	
	header.cmd	=	CMD_DATA;

	do
	{
		header.size	=	MIN(m_socketReadBufferSize, (package.size() - pos));
		
		data.clear();
		data.reserve(sizeof(header) + header.size);
		
		// Header
		data.append((const char*)&header, sizeof(header));

		// Data
		data.append((const char*)(package.constData() + pos), (int)header.size);
		
		writeBuffer.enqueue(data);
		
		notifyWrite();
		
// 		qDebug("Enqueued %d (%d) bytes for writing [0]=%u", size, data.size(), (uchar)data.constData()[2*sizeof(quint32)]);
		
		pos		+=	header.size;
	}while(pos < package.size());
}


void LocalSocketPrivate::writePackage(const QByteArray& package)
{
	Q_Q(LocalSocket);
	
	if(!m_socket)
	{
		q->close();
		return;
	}
	
	if(package.isEmpty())
		return;
	
	QByteArray	data;
	int					pos	=	0;
	struct	Header	header;
	INIT_HEADER(header);
	
	header.cmd	=	CMD_PKG;

	do
	{
		header.size	=	MIN(m_socketReadBufferSize, (package.size() - pos));
		
		data.clear();
		data.reserve(sizeof(header) + header.size);
		
		// Header
		data.append((const char*)&header, sizeof(header));

		// Data
		data.append((const char*)(package.constData() + pos), (int)header.size);
		
		writeBuffer.enqueue(data);
		notifyWrite();
		
		pos		+=	header.size;
	}while(pos < package.size());

	header.cmd		=	CMD_PKG_END;
	quint32	pkgSize	=	package.size();
	header.size	=	sizeof(pkgSize);
	
	data.clear();
	data.reserve(sizeof(header) + header.size);
	
	// Header
	data.append((const char*)&header, sizeof(header));
	// Package size
	data.append((const char*)&pkgSize, sizeof(pkgSize));
	
	writeBuffer.enqueue(data);
	
	notifyWrite();
}


void LocalSocketPrivate::writeSocketDescriptor(int socketDescriptor)
{
	writeSDescBuffer.enqueue(socketDescriptor);
	notifyWrite();
}


void LocalSocketPrivate::run()
{
	Q_Q(LocalSocket);
	LocalSocketPrivate_Worker	worker(this);
	w	=	&worker;
	
	connect(w, SIGNAL(aborted()), SLOT(quit()), Qt::DirectConnection);
	
	w->init();
	
	QThread::exec();
	
	callSlotQueued(this, "doClose");
	
	if(m_socket > 0)
	{
		::close(m_socket);
		m_socket	=	0;
	}
	
	emit(disconnected());
}


void LocalSocketPrivate::doClose()
{
	Q_Q(LocalSocket);
	q->close();
}
