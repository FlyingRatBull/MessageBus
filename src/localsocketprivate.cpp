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

#include "localsocketprivate.h"

#define HEADER_SIZE (sizeof(quint8) + sizeof(quint32) + sizeof(quint32))

LocalSocketPrivate::LocalSocketPrivate(LocalSocket * q)
	:	QObject(), m_q(q), m_readNotifier(0), m_writeNotifier(0), m_exceptionNotifier(0),
	m_currentlyWritingFileDescriptor(0), m_currentWriteDataPos(0), m_socketDescriptor(0),
	m_currentRequiredReadDataSize(0), m_isOpen(false)
{
}


LocalSocketPrivate::~LocalSocketPrivate()
{
	removeReadNotifier();
	removeWriteNotifier();
	
	if(m_currentlyWritingFileDescriptor)
		delete m_currentlyWritingFileDescriptor;
}


bool LocalSocketPrivate::waitForReadyRead(QElapsedTimer& timer, int timeout)
{
	bool	readyRead		=	false;
	bool	readyWrite	=	false;
	
	disableReadNotifier();
	disableWriteNotifier();
	disableExceptionNotifier();
	
	while(m_isOpen && (timeout > 0 ? timer.elapsed() < timeout : !readyWrite))
	{
		// Do we need to write?
		readyWrite	=	(!m_currentWriteData.isEmpty() || !m_writeBuffer.isEmpty());
		// We need to read
		readyRead		=	true;
		
		bool	ret	=	waitForReadOrWrite(readyRead, readyWrite, (timeout > 0 ? timeout - timer.elapsed() : 0));
		
		// Failed to wait
		if(!ret)
		{
			if(m_isOpen)
			{
				enableReadNotifier();
				enableWriteNotifier();
				enableExceptionNotifier();
			}
			
			return false;
		}
		
		// Read and write data if we can
		if(readyWrite)
			writeData();
		if(readyRead)
			readData();
		
		if(readyRead)
			break;
		
		readyRead		=	false;
		readyWrite	=	false;
	}
	
	enableReadNotifier();
	enableWriteNotifier();
	enableExceptionNotifier();
	
	return readyRead;
}


bool LocalSocketPrivate::waitForDataWritten(QElapsedTimer& timer, int timeout)
{
	bool	readyRead		=	false;
	bool	readyWrite	=	false;
	
	disableReadNotifier();
	disableWriteNotifier();
	disableExceptionNotifier();
	
	while(m_isOpen && (timeout > 0 ? timer.elapsed() < timeout : !readyWrite))
	{
		// We need to read and to write
		readyWrite	=	true;
		// We need to read
		readyRead		=	true;
		
		bool	ret	=	waitForReadOrWrite(readyRead, readyWrite, (timeout > 0 ? timeout - timer.elapsed() : 0));
		
		// Failed to wait
		if(!ret)
		{
			if(m_isOpen)
			{
				enableReadNotifier();
				enableWriteNotifier();
				enableExceptionNotifier();
			}
			
			return false;
		}
		
		// Read and write data if we can
		if(readyWrite)
			writeData();
		if(readyRead)
			readData();
		
		if(readyWrite)
			break;
		
		readyRead		=	false;
		readyWrite	=	false;
	}
	
	enableReadNotifier();
	enableWriteNotifier();
	enableExceptionNotifier();
	
	return readyWrite;
}


void LocalSocketPrivate::disconnectFromServer()
{
	close();
}


void LocalSocketPrivate::notifyWrite()
{
	writeData();
}


void LocalSocketPrivate::flush()
{
	while(m_isOpen && !m_writeBuffer.isEmpty() && !m_currentReadData.isEmpty())
	{
// 		qDebug("[%p] LocalSocketPrivate::flush() - flushing", this);
		writeData();
		readData();
	}
}


void LocalSocketPrivate::enableReadNotifier()
{
	if(!m_socketDescriptor)
		return;
	
	if(!m_readNotifier)
	{
		m_readNotifier	=	new QSocketNotifier(m_socketDescriptor, QSocketNotifier::Read);
		connect(m_readNotifier, SIGNAL(activated(int)), SLOT(readData()));
	}
	
	m_readNotifier->setEnabled(true);
}


void LocalSocketPrivate::disableReadNotifier()
{
	if(m_readNotifier)
		m_readNotifier->setEnabled(false);
}


void LocalSocketPrivate::removeReadNotifier()
{
	if(m_readNotifier)
	{
		m_readNotifier->setEnabled(false);
		delete	m_readNotifier;
		m_readNotifier	=	0;
	}
}


void LocalSocketPrivate::enableWriteNotifier()
{
	if(!m_socketDescriptor)
		return;
	
	if(!m_writeNotifier)
	{
		m_writeNotifier	=	new QSocketNotifier(m_socketDescriptor, QSocketNotifier::Write);
		connect(m_writeNotifier, SIGNAL(activated(int)), SLOT(writeData()));
	}
	
	m_writeNotifier->setEnabled(true);
}


void LocalSocketPrivate::disableWriteNotifier()
{
	if(m_writeNotifier)
		m_writeNotifier->setEnabled(false);
}


void LocalSocketPrivate::removeWriteNotifier()
{
	if(m_writeNotifier)
	{
		m_writeNotifier->setEnabled(false);
		delete	m_writeNotifier;
		m_writeNotifier	=	0;
	}
}


void LocalSocketPrivate::enableExceptionNotifier()
{
	if(!m_socketDescriptor)
		return;
	
	if(!m_exceptionNotifier)
	{
		m_exceptionNotifier	=	new QSocketNotifier(m_socketDescriptor, QSocketNotifier::Exception);
		connect(m_exceptionNotifier, SIGNAL(activated(int)), SLOT(exception()));
	}
	
	m_exceptionNotifier->setEnabled(true);
}


void LocalSocketPrivate::disableExceptionNotifier()
{
	if(m_exceptionNotifier)
		m_exceptionNotifier->setEnabled(false);
}


void LocalSocketPrivate::removeExceptionNotifier()
{
	if(m_exceptionNotifier)
	{
		m_exceptionNotifier->setEnabled(false);
		delete	m_exceptionNotifier;
		m_exceptionNotifier	=	0;
	}
}


void LocalSocketPrivate::setOpened()
{
	if(m_isOpen)
		return;
	
	m_isOpen	=	true;
	
	// Enable notifiers
	enableReadNotifier();
	enableWriteNotifier();
	enableExceptionNotifier();
	
	writeData();
	readData();
}


void LocalSocketPrivate::setClosed()
{
	if(!m_isOpen)
		return;
	
// 	qDebug("[%p] LocalSocketPrivate::setClosed()", this);
	
	m_isOpen	=	false;
	m_socketDescriptor	=	0;
	
	// Remove socket notifier
	removeReadNotifier();
	removeWriteNotifier();
	removeExceptionNotifier();
	
	// Clear write data
	if(m_currentlyWritingFileDescriptor)
		delete m_currentlyWritingFileDescriptor;
	m_currentlyWritingFileDescriptor	=	0;
	m_currentWriteData.clear();
	m_currentWriteDataPos	=	0;
	m_writeBuffer.clear();
	
	// Clear temporary read data
	m_currentReadData.clear();
	m_tempReadBuffer.clear();
	m_tempReadFileDescBuffer.clear();
	
	emit(m_q->disconnected());
}


void LocalSocketPrivate::setError(const QString& errorString)
{
// 	qDebug("[%p] LocalSocketPrivate::setError() - %s", this, qPrintable(errorString));
	
	m_errorString	=	errorString;
	
	emit(m_q->error(m_errorString));
	
	setClosed();
}


void LocalSocketPrivate::addReadFileDescriptor(quintptr fileDescriptor)
{
	m_tempReadFileDescBuffer.append(fileDescriptor);
	
	checkTempReadData();
}


/*
 * Header format:
 * 
 * Pos Type     Data
 * -------------------------
 * 0   quint8   Type
 * 1   quint32  Optional id
 * 5   quint32  Data size
 * -------------------------
 * Total size: 9 bytes
 */
void LocalSocketPrivate::readData()
{
// 	qDebug("[%p] LocalSocketPrivate::readData()", this);
	
	// Resize our read buffer
	m_currentReadDataBuffer.resize(readBufferSize());
	
	// Try to read data
	disableReadNotifier();
	int	numRead	=	read(m_currentReadDataBuffer.data(), m_currentReadDataBuffer.size());
	enableReadNotifier();
	
	// Handle read data
	if(numRead > 0)
	{
		m_currentReadData.append(m_currentReadDataBuffer.constData(), numRead);
		
		// Analyze read data
		while(!m_currentReadData.isEmpty())
		{
// 			qDebug("[%p] LocalSocketPrivate::readData() - reading data", this);
			
			// Read package size if available
			if(!m_currentRequiredReadDataSize && m_currentReadData.size() >= HEADER_SIZE)
			{
				memcpy((char*)&m_currentRequiredReadDataSize, m_currentReadData.constData() + (HEADER_SIZE - sizeof(quint32)), sizeof(m_currentRequiredReadDataSize));
				// Add header size
				m_currentRequiredReadDataSize	+=	HEADER_SIZE;
			}
			
			// Check if we can read a package
			if(!m_currentRequiredReadDataSize || m_currentReadData.size() < m_currentRequiredReadDataSize)
			{
// 				qDebug("[%p] LocalSocketPrivate::readData() - Cannot read package yet: %d/%d", this, m_currentReadData.size(), m_currentRequiredReadDataSize);
				break;
			}
			
// 			static	int	_r_count	=	0;
// 			_r_count++;
// 			qDebug("[%p] LocalSocketPrivate::readData() - count: %d", this, _r_count);
			
			// Read meta data
			int					dataPos	=	0;
			quint8			readVarType;
			quint32			readVarOptId;
			quint32			readVarDataSize;
			
			// Type
			memcpy((char*)&readVarType, m_currentReadData.constData() + dataPos, sizeof(readVarType));
			dataPos	+=	sizeof(readVarType);
			// Optional id
			memcpy((char*)&readVarOptId, m_currentReadData.constData() + dataPos, sizeof(readVarOptId));
			dataPos	+=	sizeof(readVarOptId);
			// Data size
			memcpy((char*)&readVarDataSize, m_currentReadData.constData() + dataPos, sizeof(readVarDataSize));
			dataPos	+=	sizeof(readVarDataSize);
			
			Variant		readVar((Variant::Type)readVarType);
			readVar.setOptionalId(readVarOptId);
			
			// Set data
			if(readVarDataSize)
			{
				readVar.setValue(m_currentReadData.mid(dataPos, readVarDataSize));
				dataPos	+=	readVarDataSize;
			}
			
			// Remove data from buffer
			m_currentReadData.remove(0, dataPos);
			m_currentRequiredReadDataSize	=	0;
			
			// Append to temporary read buffer if necessary
			if(readVar.type() == Variant::SocketDescriptor || !m_tempReadBuffer.isEmpty())
				m_tempReadBuffer.append(readVar);
			else
			{
				m_readBuffer.append(readVar);
				// We have read a package
				emit(m_q->readyRead());
			}
		}
	}
	
	checkTempReadData(true);
	
// 	qDebug("[%p] LocalSocketPrivate::~readData()", this);
}


/*
 * Header format:
 * 
 * Pos Type     Data
 * -------------------------
 * 0   quint8   Type
 * 1   quint32  Optional id
 * 5   quint32  Data size
 * -------------------------
 * Total size: 9 bytes
 */
void LocalSocketPrivate::writeData()
{
// 	qDebug("[%p] LocalSocketPrivate::writeData()", this);
	
	// Move new data into the buffer
	if(m_currentWriteData.isEmpty())
	{
		if(m_writeBuffer.isEmpty())
		{
			// Nothing to write so we don't need the notifier
			disableWriteNotifier();
// 			qDebug("[%p] LocalSocketPrivate::~writeData()", this);
			return;
		}
		
		Variant			writeVar(m_writeBuffer.takeFirst());
		QByteArray	writeVarData(writeVar.toByteArray());
		quint8			writeVarType			=	quint8(writeVar.type());
		quint32			writeVarOptId			=	writeVar.optionalId();
		quint32			writeVarDataSize	=	writeVarData.size();
		
// 		static	int	_w_count	=	0;
// 		_w_count++;
// 		qDebug("[%p] LocalSocketPrivate::writeData() - count: %d", this, _w_count);
		
		// File descriptors don't need the writeVars data as it is transferred via m_currentlyWritingFileDescriptor
		if(writeVar.type() == Variant::SocketDescriptor)
			writeVarDataSize	=	0;
		
		// Data to be written (including metadata)
		QByteArray	data;
		int					dataPos	=	0;
		
		data.resize(HEADER_SIZE);
		// Set metadata
		// type
		memcpy(data.data() + dataPos, (const char*)&writeVarType, sizeof(writeVarType));
		dataPos	+=	sizeof(writeVarType);
		// optional id
		memcpy(data.data() + dataPos, (const char*)&writeVarOptId, sizeof(writeVarOptId));
		dataPos	+=	sizeof(writeVarOptId);
		// data size
		memcpy(data.data() + dataPos, (const char*)&writeVarDataSize, sizeof(writeVarDataSize));
		dataPos	+=	sizeof(writeVarDataSize);
		
		// File descriptors don't need the writeVars data as it is transferred via m_currentlyWritingFileDescriptor
		if(writeVar.type() == Variant::SocketDescriptor)
		{
			m_currentWriteData	=	data;
			m_currentlyWritingFileDescriptor	=	new quintptr(writeVar.toSocketDescriptor());
		}
		else
			m_currentWriteData	=	data + writeVarData;
	}
	
	// Get size of data to write
	int	writeSize	=	qMin(availableWriteBufferSpace(), m_currentWriteData.size() - m_currentWriteDataPos);

	// Check if we can write data
	if(writeSize < 1)
	{
// 		qDebug("[%p] LocalSocketPrivate::~writeData()", this);
		return;
	}
	
	// Try to write data
	disableWriteNotifier();
	int	written		=	write(m_currentWriteData.constData() + m_currentWriteDataPos, writeSize, m_currentlyWritingFileDescriptor);
	
	// Data was written
	if(written > 0)
	{
// 		qDebug("[%p] LocalSocketPrivate::writeData() - written data: %d", this, written);
		
		m_currentWriteDataPos	+=	written;
		
		// Clear finished data
		if(m_currentWriteDataPos >= m_currentWriteData.size())
		{
			m_currentWriteData.clear();
			m_currentWriteDataPos	=	0;
			if(m_currentlyWritingFileDescriptor)
			{
				delete m_currentlyWritingFileDescriptor;
				m_currentlyWritingFileDescriptor	=	0;
			}
		}
	}

	// Only enable notifier if we have data to write
	if(!m_currentWriteData.isEmpty() || !m_writeBuffer.isEmpty())
		enableWriteNotifier();
	
// 	qDebug("[%p] LocalSocketPrivate::~writeData()", this);
}


void LocalSocketPrivate::exception()
{
// 	qDebug("[%p] LocalSocketPrivate::exception()", this);
	
	// Just try to read data and we will get the correct behaviour
	readData();
}


void LocalSocketPrivate::checkTempReadData(bool required)
{
// 	qDebug("[%p] LocalSocketPrivate::checkTempReadData()", this);

	while(!m_tempReadBuffer.isEmpty())
	{
// 		qDebug("[%p] LocalSocketPrivate::checkTempReadData() - checking", this);
		
		if(m_tempReadBuffer.first().type() == Variant::SocketDescriptor)
		{
			if(m_tempReadFileDescBuffer.isEmpty() && !required)
				break;
		}
		
		Variant	src(m_tempReadBuffer.takeFirst());
		
		// Pop up normal data
		if(src.type() != Variant::SocketDescriptor)
			m_readBuffer.append(src);
		else
		{
			quintptr	fileDescriptor	=	0;
			
			if(!m_tempReadFileDescBuffer.isEmpty())
				fileDescriptor	=	m_tempReadFileDescBuffer.takeFirst();
// 			else
// 				qDebug("=== using 0");
			
			Variant	package(Variant::fromSocketDescriptor(fileDescriptor));
			
			package.setOptionalId(src.optionalId());
			m_readBuffer.append(package);
		}
		
		emit(m_q->readyRead());
	}
}
