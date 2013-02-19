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

#include "localsocket.h"

#include "localsocketprivate_thread.h"

#include "unittests/logger.h"

LocalSocket::LocalSocket(QObject * parent)
	:	QIODevice(parent), d_ptr(new LocalSocketPrivate_Thread(this))
{
	connect(d_ptr, SIGNAL(connected()), SIGNAL(connected()));
	connect(d_ptr, SIGNAL(disconnected()), SIGNAL(disconnected()));
	connect(d_ptr, SIGNAL(stateChanged(LocalSocket::LocalSocketState)), SIGNAL(stateChanged(LocalSocket::LocalSocketState)));
	connect(d_ptr, SIGNAL(error(LocalSocket::LocalSocketError)), SIGNAL(error(LocalSocket::LocalSocketError)));
	
	connect(d_ptr, SIGNAL(readyReadPackage()), SIGNAL(readyReadPackage()));
	connect(d_ptr, SIGNAL(readyReadSocketDescriptor()), SIGNAL(readyReadSocketDescriptor()));
	connect(d_ptr, SIGNAL(bytesWritten(qint64)), SIGNAL(bytesWritten(qint64)));
	connect(d_ptr, SIGNAL(packageWritten()), SIGNAL(packageWritten()));
	connect(d_ptr, SIGNAL(socketDescriptorWritten(quintptr)), SIGNAL(socketDescriptorWritten(quintptr)));
	connect(d_ptr, SIGNAL(readyRead()), SIGNAL(readyRead()));
}


LocalSocket::~LocalSocket()
{
	if(isOpen())
		close();
	
	delete d_ptr;
}


void LocalSocket::connectToServer(const QString& name, QIODevice::OpenMode mode)
{
	if(d_ptr->state != UnconnectedState)
	{
		setErrorString("Not in unconnected state when trying to connect!");
		emit(error(ConnectionError));
		return;
	}
	
	d_ptr->start();
	
	// Wait for LocalSocketPrivate instance to be ready
	QReadLocker		lsPrivateLocker(&d_ptr->lsPrivateLock);
	
	if(!d_ptr->lsPrivate)
		d_ptr->lsPrivateChanged.wait(&d_ptr->lsPrivateLock, 30000);
	
	// Fail if there is no instance
	if(!d_ptr->lsPrivate)
	{
		setErrorString("Internal error when trying to connect!");
		emit(error(ConnectionError));
		d_ptr->close();
		return;
	}
	
	{
		QWriteLocker		stateLocker(&d_ptr->stateLock);
		d_ptr->state	=	ConnectingState;
	}
	
	// Create filename
	QDir		tmpDir(QDir::temp());
	QString	filename(tmpDir.absoluteFilePath("LocalSocket_" + QString::fromAscii(QCryptographicHash::hash(name.toUtf8(), QCryptographicHash::Sha1).toHex()) + ".sock"));
	
	d_ptr->serverFilename	=	filename;
	
// 	Logger::log("Bytes Written (LocalSocket)", 0, "cS", "connectToServer");
	
	// Send connect event
	LocalSocketPrivateEvent	*	event	=	new LocalSocketPrivateEvent(LocalSocketPrivateEvent::Connect);
	event->connectTarget	=	filename;
	event->connectMode		=	mode;
	QCoreApplication::postEvent(d_ptr->lsPrivate, event);
	QCoreApplication::flush();
}


bool LocalSocket::setSocketDescriptor(quintptr socketDescriptor, LocalSocket::LocalSocketState socketState, QIODevice::OpenMode openMode)
{
	if(d_ptr->state != UnconnectedState)
		return false;
	
	d_ptr->start();
	
	// Wait for LocalSocketPrivate instance to be ready
	QReadLocker		lsPrivateLocker(&d_ptr->lsPrivateLock);
	
	if(!d_ptr->lsPrivate)
		d_ptr->lsPrivateChanged.wait(&d_ptr->lsPrivateLock, 30000);
	
	// Fail if there is no instance
	if(!d_ptr->lsPrivate)
	{
		d_ptr->close();
		return false;
	}
	
	{
		QWriteLocker		stateLocker(&d_ptr->stateLock);
		d_ptr->state	=	socketState;
	}
	
	// Send connect event
	LocalSocketPrivateEvent	*	event	=	new LocalSocketPrivateEvent(LocalSocketPrivateEvent::Connect);
	event->connectTarget						=	Variant::fromSocketDescriptor(socketDescriptor);
	event->socketDescriptorOpen			=	(socketState == ConnectedState || socketState == ConnectingState);
	event->connectMode							=	openMode;
	
// 	Logger::log("Bytes Written (LocalSocket)", 0, "sS", "setSocketDescriptor");

	// Locking state before posting the event is important! So wait() will always work!
	QReadLocker		stateLocker(&d_ptr->stateLock);
	QCoreApplication::postEvent(d_ptr->lsPrivate, event);
	d_ptr->isStateChanged.wait(&d_ptr->stateLock, 30000);

	return !d_ptr->lsPrivate->hasError();
}


bool LocalSocket::waitForConnected(int msecs)
{
	// Lock state
	QReadLocker		stateLocker(&d_ptr->stateLock);
	
	if(d_ptr->state == ConnectingState)
		d_ptr->isStateChanged.wait(&d_ptr->stateLock, msecs);
	
	return (d_ptr->state == ConnectedState);
}


bool LocalSocket::waitForDisconnected(int msecs)
{
	// Lock state
	QReadLocker		stateLocker(&d_ptr->stateLock);
	
	if(d_ptr->state == ClosingState)
		d_ptr->isStateChanged.wait(&d_ptr->stateLock, msecs);
	
	return (d_ptr->state == UnconnectedState);
}


bool LocalSocket::canReadLine() const
{
	// Wait for LocalSocketPrivate instance to be ready
	QReadLocker		lsPrivateLocker(&d_ptr->lsPrivateLock);
	
	if(!d_ptr->lsPrivate)
		return false;
	
	return QIODevice::canReadLine() || d_ptr->lsPrivate->canReadLine();
}


QString LocalSocket::serverName() const
{
	if(d_ptr->serverFilename.isEmpty())
		return QString();
	
	QFileInfo		info(d_ptr->serverFilename);
	
	return info.fileName();
}


QString LocalSocket::fullServerName() const
{
	return d_ptr->serverFilename;
}


LocalSocket::LocalSocketError LocalSocket::error() const
{
	return d_ptr->lastError;
}


quintptr LocalSocket::socketDescriptor() const
{
	// Wait for LocalSocketPrivate instance to be ready
	QReadLocker		lsPrivateLocker(&d_ptr->lsPrivateLock);
	
	if(!d_ptr->lsPrivate)
		d_ptr->lsPrivateChanged.wait(&d_ptr->lsPrivateLock, 30000);
	
	if(d_ptr->lsPrivate)
		return d_ptr->lsPrivate->socketDescriptor();
	else
		return 0;
}


qint64 LocalSocket::readBufferSize() const
{
	QReadLocker			lsPrivateLocker(&d_ptr->lsPrivateLock);
	
	if(d_ptr->lsPrivate)
		return d_ptr->lsPrivate->readBufferSize();
	else
		return d_ptr->readBufferSize;
}


void LocalSocket::setReadBufferSize(qint64 size)
{
	QReadLocker			lsPrivateLocker(&d_ptr->lsPrivateLock);
	
	if(d_ptr->lsPrivate)
		d_ptr->lsPrivate->setReadBufferSize(size);
	
	d_ptr->lsPrivate->setReadBufferSize(size);
}


bool LocalSocket::isValid() const
{
	return (d_ptr->state == ConnectedState);
}


bool LocalSocket::flush()
{
	if(!isValid())
		return false;
	
	// Wait for LocalSocketPrivate instance to be ready
	QReadLocker		lsPrivateLocker(&d_ptr->lsPrivateLock);
	
	if(!d_ptr->lsPrivate)
		return false;
	
	// Send flush event
	LocalSocketPrivateEvent		event(LocalSocketPrivateEvent::Flush);
	QCoreApplication::sendEvent(d_ptr->lsPrivate, &event);
	
	// The event is accepted if data was written
	return event.isAccepted();
}


LocalSocket::LocalSocketState LocalSocket::state() const
{
	return d_ptr->state;
}


void LocalSocket::abort()
{
	if(!isOpen())
		return;
	
	// Wait for LocalSocketPrivate instance to be ready
	QReadLocker		lsPrivateLocker(&d_ptr->lsPrivateLock);
	
	if(!d_ptr->lsPrivate)
		return;
	
	{
		QWriteLocker	locker(&d_ptr->stateLock);
		// Also prevents from data being written to the write buffers
		d_ptr->state	=	ClosingState;
	}
	
	// Close the socket synchronously
	LocalSocketPrivateEvent		event(LocalSocketPrivateEvent::Disconnect);
	QCoreApplication::sendEvent(d_ptr->lsPrivate, &event);
	
	// We don't need the lock anymore and it must be unlocked for close() to work!
	lsPrivateLocker.unlock();
	
	// Close the thread
	d_ptr->close();
	
	QIODevice::close();
}


void LocalSocket::disconnectFromServer()
{
	// Wait for LocalSocketPrivate instance to be ready
	QReadLocker		lsPrivateLocker(&d_ptr->lsPrivateLock);
	
	if(!d_ptr->lsPrivate)
		return;
	
	// aboutToClose gets only emitted here as abort() should close the connection emediately
	emit(aboutToClose());

	// Also prevents from data being written to the write buffers
	d_ptr->state	=	ClosingState;
	
	// Flush data
	QCoreApplication::postEvent(d_ptr->lsPrivate, new LocalSocketPrivateEvent(LocalSocketPrivateEvent::Flush));
	
	// Disconnect
	QCoreApplication::sendEvent(d_ptr->lsPrivate, new LocalSocketPrivateEvent(LocalSocketPrivateEvent::Disconnect));
}


qint64 LocalSocket::bytesAvailable() const
{
	// Wait for LocalSocketPrivate instance to be ready
	QReadLocker		lsPrivateLocker(&d_ptr->lsPrivateLock);
	
	if(!d_ptr->lsPrivate)
		return 0;
	
	// Return buffer size of LocalSocketPrivate and of QIODevice itself
	return d_ptr->lsPrivate->m_readBuffer.size() + QIODevice::bytesAvailable();
}


qint64 LocalSocket::packagesAvailable() const
{
	// Wait for LocalSocketPrivate instance to be ready
	QReadLocker		lsPrivateLocker(&d_ptr->lsPrivateLock);
	
	if(!d_ptr->lsPrivate)
		return 0;
	
	return d_ptr->lsPrivate->m_readPkgBuffer.count();
}


qint64 LocalSocket::socketDescriptorsAvailable() const
{
	// Wait for LocalSocketPrivate instance to be ready
	QReadLocker		lsPrivateLocker(&d_ptr->lsPrivateLock);
	
	if(!d_ptr->lsPrivate)
		return 0;
	
	return d_ptr->lsPrivate->m_readSDescBuffer.count();
}


QByteArray LocalSocket::readPackage()
{
	// Wait for LocalSocketPrivate instance to be ready
	QReadLocker		lsPrivateLocker(&d_ptr->lsPrivateLock);
	
	if(!d_ptr->lsPrivate)
		return QByteArray();
	
	if(d_ptr->lsPrivate->m_readPkgBuffer.isEmpty())
		return QByteArray();
	
	return d_ptr->lsPrivate->m_readPkgBuffer.dequeue();
}


quintptr LocalSocket::readSocketDescriptor()
{
	// Wait for LocalSocketPrivate instance to be ready
	QReadLocker		lsPrivateLocker(&d_ptr->lsPrivateLock);
	
	if(!d_ptr->lsPrivate)
		return 0;
	
	if(d_ptr->lsPrivate->m_readSDescBuffer.isEmpty())
		return 0;
	
	return d_ptr->lsPrivate->m_readSDescBuffer.dequeue();
}


qint64 LocalSocket::readData(char *data, qint64 maxlen)
{
	// Wait for LocalSocketPrivate instance to be ready
	QReadLocker		lsPrivateLocker(&d_ptr->lsPrivateLock);
	
	if(!d_ptr->lsPrivate)
		return -1;
	
	if(d_ptr->lsPrivate->m_readBuffer.isEmpty())
		return 0;
	
	// We cast to uint -> Check bounds
	if(maxlen > UINT_MAX)
		maxlen	=	UINT_MAX;
	
	// For debugging
		
// 	Logger::log("Read buffer size (LocalSocketPrivate)", d_ptr->lsPrivate->m_readBuffer.size());
	qint64	len	=	(qint64)d_ptr->lsPrivate->m_readBuffer.dequeue(data, maxlen);
// 	qDebug("LocalSocket::readData(...) - Read %lld bytes", len);
// 	static	quint64	LSTotalRead	=	0;
// 	LSTotalRead	+=	len;
// 	Logger::log("Bytes read (LocalSocket)", LSTotalRead);
// 	Logger::log("Read buffer size (LocalSocketPrivate)", d_ptr->lsPrivate->m_readBuffer.size());
	
	return len;
}


bool LocalSocket::writePackage(const QByteArray& package)
{
	if(package.size() < 1)
		return false;
	
	// Wait for LocalSocketPrivate instance to be ready
	QReadLocker		lsPrivateLocker(&d_ptr->lsPrivateLock);
	
	if(!d_ptr->lsPrivate)
		return false;
	
	d_ptr->lsPrivate->addToPackageWriteBuffer(package);
	
	// postEvent takes responsability for deletion of the event
	QCoreApplication::postEvent(d_ptr->lsPrivate, new LocalSocketPrivateEvent(LocalSocketPrivateEvent::WritePackage));
	QCoreApplication::flush();
	
	return true;
}


bool LocalSocket::writeSocketDescriptor(quintptr socketDescriptor)
{
	if(socketDescriptor < 1)
		return false;
	
	// Wait for LocalSocketPrivate instance to be ready
	QReadLocker		lsPrivateLocker(&d_ptr->lsPrivateLock);
	
	if(!d_ptr->lsPrivate)
		return false;
	
	d_ptr->lsPrivate->addToWriteBuffer(socketDescriptor);
	
	// postEvent takes responsability for deletion of the event
	QCoreApplication::postEvent(d_ptr->lsPrivate, new LocalSocketPrivateEvent(LocalSocketPrivateEvent::WriteSocketDescriptor));
	QCoreApplication::flush();
	
	return true;
}


qint64 LocalSocket::writeData(const char *data, qint64 len)
{
// 	qDebug("LocalSocket::writeData( ... )");
	
	if(len < 1)
		return 0;
	
	// Wait for LocalSocketPrivate instance to be ready
	QReadLocker		lsPrivateLocker(&d_ptr->lsPrivateLock);
	
	if(!d_ptr->lsPrivate)
		return 0;
	
	///@todo Use max size for write buffer and return the actual size that was added to the write buffer
	
	// We can only pass INT -> Check size
	if(len > INT_MAX)
		len	=	INT_MAX;
	
	d_ptr->lsPrivate->addToWriteBuffer(data, (int)len);
	
	// For debugging
	
// 	static quint64	totalWrittenLSWriteData	=	0;
// 	totalWrittenLSWriteData	+=	len;
// 	qDebug("total-written (LS:writeData): %llu", totalWrittenLSWriteData);
// 	Logger::log("Bytes Written (LocalSocket)", totalWrittenLSWriteData);
	
	// postEvent takes responsability for deletion of the event
	QCoreApplication::postEvent(d_ptr->lsPrivate, new LocalSocketPrivateEvent(LocalSocketPrivateEvent::WriteData));
	QCoreApplication::flush();
	
	return len;
}


qint64 LocalSocket::bytesToWrite() const
{
	// Wait for LocalSocketPrivate instance to be ready
	QReadLocker		lsPrivateLocker(&d_ptr->lsPrivateLock);
	
	if(!d_ptr->lsPrivate)
		return 0;
	
	return d_ptr->lsPrivate->writeBufferSize() + QIODevice::bytesToWrite();
}


qint64 LocalSocket::packagesToWrite() const
{
	// Wait for LocalSocketPrivate instance to be ready
	QReadLocker		lsPrivateLocker(&d_ptr->lsPrivateLock);
	
	if(!d_ptr->lsPrivate)
		return 0;
	
	return d_ptr->lsPrivate->writePackageBufferSize();
}


qint64 LocalSocket::socketDescriptorsToWrite() const
{
	// Wait for LocalSocketPrivate instance to be ready
	QReadLocker		lsPrivateLocker(&d_ptr->lsPrivateLock);
	
	if(!d_ptr->lsPrivate)
		return 0;
	
	return d_ptr->lsPrivate->writeSocketDescriptorBufferSize();
}


bool LocalSocket::waitForBytesWritten(int msecs)
{
	if(d_ptr->state != ConnectingState && d_ptr->state != ConnectedState)
		return false;
	
	// Lock state
	QReadLocker		stateLocker(&d_ptr->stateLock);
	
	if(!d_ptr->lsPrivate || d_ptr->lsPrivate->writeBufferSize() < 1)
		return false;
	
	// Lock bytes written lock
	QReadLocker		bytesWrittenLocker(&d_ptr->bytesWrittenLock);
	
	// Wait for bytes to be written
	return d_ptr->areBytesWritten.wait(&d_ptr->bytesWrittenLock, msecs);
}


bool LocalSocket::waitForPackageWritten(int msecs)
{
	if(d_ptr->state != ConnectingState && d_ptr->state != ConnectedState)
		return false;
	
	// Lock state
	QReadLocker		stateLocker(&d_ptr->stateLock);
	
	if(!d_ptr->lsPrivate || d_ptr->lsPrivate->writePackageBufferSize() < 1)
		return false;
	
	// Lock bytes written lock
	QReadLocker		packageWrittenLocker(&d_ptr->packageWrittenLock);
	
	// Wait for bytes to be written
	return d_ptr->isPackageWritten.wait(&d_ptr->packageWrittenLock, msecs);
}


bool LocalSocket::waitForSocketDescriptorWritten(int msecs)
{
	if(d_ptr->state != ConnectingState && d_ptr->state != ConnectedState)
		return false;
	
	// Lock state
	QReadLocker		stateLocker(&d_ptr->stateLock);
	
	if(!d_ptr->lsPrivate || d_ptr->lsPrivate->writeSocketDescriptorBufferSize() < 1)
		return false;
	
	// Lock bytes written lock
	QReadLocker		socketDescriptorWrittenLocker(&d_ptr->socketDescriptorWrittenLock);
	
	// Wait for bytes to be written
	return d_ptr->isSocketDescriptorWritten.wait(&d_ptr->socketDescriptorWrittenLock, msecs);
}


bool LocalSocket::waitForReadyRead(int msecs)
{
	if(d_ptr->state != ConnectingState && d_ptr->state != ConnectedState)
		return false;
	
	if(d_ptr->lsPrivate->m_readBuffer.size() + QIODevice::bytesAvailable() > 0)
		return true;
	
	// Don't wait for the signal but wait for data to be ready for reading
	return d_ptr->lsPrivate->m_readBuffer.waitForNonEmpty(msecs);
	
// 	// Lock state
// 	QReadLocker		stateLocker(&d_ptr->stateLock);
// 	
// 	if(!d_ptr->lsPrivate)
// 		return false;
// 	
// 	// Lock ready read lock
// 	QReadLocker		readyReadWrittenLocker(&d_ptr->readyReadLock);
// 	
// 	if(d_ptr->lsPrivate->m_readBuffer.size() + QIODevice::bytesAvailable() > 0)
// 		return true;
// 	
// 	// Wait for bytes to be written
// 	return d_ptr->isReadyRead.wait(&d_ptr->readyReadLock, msecs);
}


bool LocalSocket::waitForReadyReadPackage(int msecs)
{
	if(d_ptr->state != ConnectingState && d_ptr->state != ConnectedState)
		return false;
	
	// Lock state
	QReadLocker		stateLocker(&d_ptr->stateLock);
	
	if(!d_ptr->lsPrivate)
		return false;
	
	// Lock ready read lock
	QReadLocker		readyReadPackageWrittenLocker(&d_ptr->readyReadPackageLock);
	
	// Wait for bytes to be written
	return d_ptr->isReadyReadPackage.wait(&d_ptr->readyReadPackageLock, msecs);
}


bool LocalSocket::waitForReadyReadSocketDescriptor(int msecs)
{
	if(d_ptr->state != ConnectingState && d_ptr->state != ConnectedState)
		return false;
	
	// Lock state
	QReadLocker		stateLocker(&d_ptr->stateLock);
	
	if(!d_ptr->lsPrivate)
		return false;
	
	// Lock ready read lock
	QReadLocker		readyReadSocketDescriptorWrittenLocker(&d_ptr->readyReadSocketDescriptorLock);
	
	// Wait for bytes to be written
	return d_ptr->isReadyReadSocketDescriptor.wait(&d_ptr->readyReadSocketDescriptorLock, msecs);
}
