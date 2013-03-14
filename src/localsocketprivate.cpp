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

#include "localsocketprivate.h"

#include <QtCore/QCoreApplication>

#include "variant.h"

#include "unittests/logger.h"


QEvent::Type LocalSocketPrivateEvent::LocalSocketPrivateType	=	QEvent::None;


/*
 * Definitions: Last written type
 */
#define LAST_WRITTEN_NONE		0
#define LAST_WRITTEN_MIN		1

#define LAST_WRITTEN_DATA		1
#define LAST_WRITTEN_PKG		2
#define LAST_WRITTEN_SDESC	3

#define LAST_WRITTEN_MAX		3

/*
 * Definitions: Commands
 */
#define CMD_DATA			0x10		// Raw data
#define CMD_PKG				0x11		// Data portion of package
#define CMD_PKG_END		0x12		// Last data portion of package (completes the package)
#define CMD_SOCK			0x13		// Socket descriptor
#define CMD_SOCK_SUC	0x14		// Successfull receival of socket descriptor

#define CMD_MIN			0x10
#define CMD_MAX			0x14
///@todo Implement command for informing about the buffer sizes

/*
 * Functions
 */
#define INIT_HEADER(_header,_size,_cmd,_dataPtr) {(_header).cmd=qToBigEndian<quint32>(_cmd);(_header).size=qToBigEndian<quint32>(_size);(_header).crc16=qChecksum((_dataPtr),(_size));(_header).bigEndian=true;}
#define CORRECT_HEADER(_header) {if((_header).bigEndian){(_header).cmd=qFromBigEndian<quint32>((_header).cmd);(_header).size=qFromBigEndian<quint32>((_header).size);(_header).bigEndian=false;}}



LocalSocketPrivate::LocalSocketPrivate(LocalSocket * q)
:	QObject(0), m_isOpen(false), m_openMode(QIODevice::ReadWrite), m_hasError(false), m_readBufferSize(0), m_remainingReadBytes(Header_size), m_writeBufferSize(0), m_writeBufferSizeNet(0),
m_lastWrittenType(LAST_WRITTEN_NONE), m_currentWritePackagePosition(0)
{
	setWriteBufferSize(8192);
}


LocalSocketPrivate::~LocalSocketPrivate()
{
	// close() must not be called from here as the subclass is already destroyed at this point!
}


QIODevice::OpenMode LocalSocketPrivate::openMode() const
{
	return m_openMode;
}


bool LocalSocketPrivate::hasError() const
{
	return m_hasError;
}


void LocalSocketPrivate::addToWriteBuffer(const char *data, int size)
{
// 	qDebug("[0x%08X] addToWriteBuffer()", int(QCoreApplication::instance()));

// 	static quint64	totalWrittenLSPEnqueue	=	0;
// 	totalWrittenLSPEnqueue	+=	size;
// 	qDebug("total-written (LSP:enqueue): %llu", totalWrittenLSPEnqueue);

	m_writeDataBuffer.enqueue(data, size);
}


void LocalSocketPrivate::addToPackageWriteBuffer(const QByteArray& package)
{
	m_writePkgBuffer.enqueue(package);
}


void LocalSocketPrivate::addToWriteBuffer(quintptr socketDescriptor)
{
	m_writeSDescBuffer.enqueue(socketDescriptor);
}


qint64 LocalSocketPrivate::readBufferSize() const
{
	return m_readBufferSize;
}


void LocalSocketPrivate::setReadBufferSize(qint64 size)
{
	///@todo Set buffer size of read buffer
	///@todo Implement equal functionality for packages and socket descriptors
// 	m_readBufferSize	=	size;

	m_tmpReadBuffer.reserve(size * 2);
}


qint64 LocalSocketPrivate::writeBufferSize() const
{
	return m_writeDataBuffer.size();
}


qint64 LocalSocketPrivate::writePackageBufferSize() const
{
	return m_writePkgBuffer.count();
}


qint64 LocalSocketPrivate::writeSocketDescriptorBufferSize() const
{
	return m_writeSDescBuffer.count();
}


bool LocalSocketPrivate::canReadLine() const
{
	return m_readBuffer.contains('\n');
}


bool LocalSocketPrivate::event(QEvent *event)
{
	if(event->type() == LocalSocketPrivateEvent::LocalSocketPrivateType)
	{
		LocalSocketPrivateEvent		*	ev	=	dynamic_cast<LocalSocketPrivateEvent*>(event);
		
		if(!ev)
			return false;
		
		switch(ev->socketEventType)
		{
			/*
			 * Sent by:
			 * 
			 * connectToServer() / setSocketDescriptor()
			 */
			case LocalSocketPrivateEvent::Connect:
			{
				m_openMode	=	ev->connectMode;
				
				if(ev->connectTarget.type() == Variant::String)
					open(ev->connectTarget.toString(), ev->connectMode);
				else if(ev->connectTarget.type() == Variant::SocketDescriptor)
					open(ev->connectTarget.toSocketDescriptor(), ev->socketDescriptorOpen, ev->connectMode);
				
				bool	result	=	false;
				
				if(isOpen())
				{
					result	=	true;
					
					// Write data in buffers
					if((!m_writeDataBuffer.isEmpty() || !m_writePkgBuffer.isEmpty() || !m_writeSDescBuffer.isEmpty() || !m_currentWritePackage.isEmpty()))
						QCoreApplication::postEvent(this, new LocalSocketPrivateEvent(LocalSocketPrivateEvent::Flush));
				}
				
				// Here either setError() must have been called or setOpen() for waitForConnected() to work correctly
				
				ev->setAccepted(result);
			}break;
			
			/*
			 * Sent by disconnectFromServer() / abort()
			 */
			case LocalSocketPrivateEvent::Disconnect:
			{
				close();
				
				ev->setAccepted(true);
			}break;
			
			/*
			 * Sent by disconnectFromServer() / flush()
			 */
			case LocalSocketPrivateEvent::Flush:
			{
// 				Logger::log("Bytes read (LocalSocketPrivate)", totalReadLS, "F", "Flush");
				
				if(!isOpen() || !writeDataRotated())
					ev->setAccepted(false);
				else
					ev->setAccepted(true);
			}break;
			
			case LocalSocketPrivateEvent::WriteSocketDescriptor:
			case LocalSocketPrivateEvent::WritePackage:
			case LocalSocketPrivateEvent::WriteData:
			{
// 				Logger::log("Bytes read (LocalSocketPrivate)", totalReadLS, "W", "Write");
				
				if(!isOpen() || !writeDataRotated())
					ev->setAccepted(false);
				else
					ev->setAccepted(true);
			}break;
			
			default:
			{
				ev->setAccepted(false);
			}break;
		}
		
		// We still have data to send -> send new event
		if(isOpen() && (!m_writeDataBuffer.isEmpty() || !m_writePkgBuffer.isEmpty() || !m_writeSDescBuffer.isEmpty() || !m_currentWritePackage.isEmpty()))
			QCoreApplication::postEvent(this, new LocalSocketPrivateEvent(LocalSocketPrivateEvent::Flush));

		// Don't return whether it was handled or not because the system can't handle our custom event anyways
		return true;
	}
	else
		return QObject::event(event);
}


void LocalSocketPrivate::setWriteBufferSize(quint32 size)
{
	// Minimum: 64 bytes
	m_writeBufferSize			=	qMax((quint32)64, size);
	m_writeBufferSizeNet	=	m_writeBufferSize - Header_size;
	
	///@test Test TsDataQueue::setPartSize()
// 	m_writeDataBuffer.setPartSize(m_writeBufferSize - Header_size);
	
	m_writeBuffer.resize(m_writeBufferSize);
}


void LocalSocketPrivate::addReadData(const char *src, int size)
{
	// Append data to temporary read data for later analyzation
// 	qDebug("[%p] addReadData() - m_tmpReadBuffer.size(): %d (+%d)", QCoreApplication::instance(), m_tmpReadBuffer.size(), size);
	m_tmpReadBuffer.append(src, size);
// 	qDebug("[%p] ok", QCoreApplication::instance());
	
// 	qDebug("[%p] addReadData() - m_tmpReadBuffer.size(): %d", QCoreApplication::instance(), m_tmpReadBuffer.size());
	
	QByteArray					readData;
	QList<QByteArray>		readPackages;

	// Try to read data portions
	while(!m_tmpReadBuffer.isEmpty())
	{
		m_remainingReadBytes	=	Header_size;
		
		if(m_tmpReadBuffer.size() < Header_size)
		{
			m_remainingReadBytes	=	Header_size - m_tmpReadBuffer.size();
			break;
		}
		
		// Read header
		struct Header	*	header	=	(Header*)m_tmpReadBuffer.constData();
		
		// Converts from big endian if needed
		CORRECT_HEADER(*header);
		
		// Check command - size can be invalid if command is already invalid
		if(header->cmd > CMD_MAX || header->cmd < CMD_MIN)
		{
			qDebug("Invalid data header!");
			setError(LocalSocket::SocketResourceError, "Invalid data header!");
			break;
		}
		
// 		qDebug("header.cmd = 0x%08X", header.cmd);
		
		// Check size
		if(m_tmpReadBuffer.size() < header->size + Header_size)
		{
			// Add additional size of header as we almost always want at least an header (Except when an socket descriptor follows)
			m_remainingReadBytes	=	(header->size + Header_size) - m_tmpReadBuffer.size() + (header->cmd != CMD_SOCK ? Header_size : 0);
			break;
		}
		
// 		qDebug("header.size = %u", header.size);
		
		// Check crc
		if(header->crc16 != qChecksum(m_tmpReadBuffer.constData() + Header_size, header->size))
		{
// 			qDebug("Invalid CRC checksum:\n\theader.crc16 = 0X%04X\n\tcrc15        = 0x%04X", header.crc16, qChecksum(m_tmpReadBuffer.constData() + sizeof(header), header.size));
			setError(LocalSocket::SocketResourceError, "Invalid CRC checksum!");
			break;
		}
		
		QByteArray	data(m_tmpReadBuffer.mid(Header_size, header->size));
		
		// For debugging
		
// 		qDebug("Got data size: %d", data.size());
// 		static quint64	totalReadLS	=	0;
// 		totalReadLS	+=	data.size() + sizeof(header);
// 		qDebug("total-read (ls): %llu", totalReadLS);
// 		Logger::log("Bytes read (LocalSocketPrivate)", totalReadLS);
		
// 		qDebug("??? sizeof(header): %d; header.size: %d; total: %d", Header_size, header->size, Header_size + header->size);
		
		switch(header->cmd)
		{
			case CMD_DATA:
			{
				///@todo Implement max size in TsDataQueue and waitForNonFull() and use them here
				readData.append(data);
			}break;
			
			case CMD_PKG:
			{
				m_currentReadPackage.append(data);
			}break;
			
			case CMD_PKG_END:
			{
				m_currentReadPackage.append(data);
				readPackages.append(m_currentReadPackage);
				m_currentReadPackage.clear();
				
// 				static	int	totalPkgEnd	=	0;
// 				totalPkgEnd++;
// 				qDebug("totalPkgEnd: %d", totalPkgEnd);
			}break;
			
			case CMD_SOCK:
			{
				// Notify the implementation that an socket descriptor is ready for receival
				notifyReadSocketDescriptor();
			}break;
			
			case CMD_SOCK_SUC:
			{
				// An socket descriptor has successfully been written to the peer
				quintptr		sdData						=	qFromBigEndian<quintptr>(*((quintptr*)(data.constData())));
				
				emit(socketDescriptorWritten(sdData));
			}break;
		}
		
		// Remove from temporary buffer
		m_tmpReadBuffer	=	m_tmpReadBuffer.mid(Header_size + header->size);
	}
	
	if(!readData.isEmpty())
	{
		// For debugging
		
// 		static	quint64	LSPaddRead	=	0;
// 		LSPaddRead	+=	readData.size();
// 		Logger::log("Read buffer size (LocalSocketPrivate)", m_readBuffer.size());
// 		Logger::log("Total Bytes added for reading (LocalSocketPrivate)", LSPaddRead);
// 		Logger::log("Bytes added for reading (LocalSocketPrivate)", readData.size());
		m_readBuffer.enqueue(readData);
// 		Logger::log("Read buffer size (LocalSocketPrivate)", m_readBuffer.size());
		emit(readyRead());
	}
	
	if(!readPackages.isEmpty())
	{
		foreach(const QByteArray& pkg, readPackages)
		{
			m_readPkgBuffer.enqueue(pkg);
			emit(readyReadPackage());
			
// 			static	int	totalReadyReadPkg	=	0;
// 			totalReadyReadPkg++;
// 			qDebug("totalReadyReadPkg: %d", totalReadyReadPkg);
		}
	}
	
// 	qDebug("??? ~addReadData() - m_tmpReadBuffer.size() = %d", m_tmpReadBuffer.size());
}


void LocalSocketPrivate::addReadSocketDescriptor(quintptr socketDescriptor, quintptr peerSocketDescriptor)
{
	Q_ASSERT(socketDescriptor > 0);
	
	// Following data is at least one header
	m_remainingReadBytes	=	Header_size;
	
	m_readSDescBuffer.enqueue(socketDescriptor);
	
	// Initialize header data
// 	QByteArray			data;
// 	data.resize(Header_size + sizeof(peerSocketDescriptor));
	
	// Set data
	quintptr	pSD				=	qToBigEndian<quintptr>(peerSocketDescriptor);
	quint32		totalSize	=	sizeof(pSD) + Header_size;
	memcpy(m_writeBuffer.data() + Header_size, &pSD, sizeof(pSD));
	
	// Initialize header
	// This must happen after copying the data as the CRC checksum gets generated here
	struct Header	*	header	=	(struct Header*)m_writeBuffer.data();
	INIT_HEADER((*header), sizeof(pSD), CMD_SOCK_SUC, m_writeBuffer.constData() + Header_size);
	
	quint32	written	=	0;
	
	while(isOpen() && written < totalSize)
		written	+=	writeData(m_writeBuffer.constData() + written, totalSize - written);
	
// 	qDebug("!!! Written SOCK_SUC !!!");
	
	// Data has been written -> Remove from buffer
	if(written != totalSize)
	{
		// An error must have already been set by the implementation
		return;
	}
	
	emit(readyReadSocketDescriptor());
}


void LocalSocketPrivate::setOpened()
{
	if(m_isOpen)
		return;
	
	emit(connected());
	m_isOpen	=	true;
}


void LocalSocketPrivate::setClosed()
{
// 	qDebug("--- setClosed()");
	
	if(!m_isOpen)
		return;
	
	emit(disconnected());
	m_isOpen	=	false;
	
	m_currentReadPackage.clear();
	m_currentWritePackage.clear();
	
	m_writeDataBuffer.clear();
	m_writePkgBuffer.clear();
	m_writeSDescBuffer.clear();
}


void LocalSocketPrivate::setError(LocalSocket::LocalSocketError socketError, const QString &errorText)
{
// 	qDebug("--- setError(): %s", qPrintable(errorText));
	
// 	Logger::log("Bytes written (LocalSocketPrivate)", 0, "E", "Error:\n" + errorText);
	
	m_hasError	=	true;
	
	emit(error(socketError, errorText));
	close();
}


quint32 LocalSocketPrivate::remainingReadBytes() const
{
	// If tempted to set m_remainingReadBytes to 0, set it to Header_size!
	Q_ASSERT_X(m_remainingReadBytes != 0, __FILE__ ":" "remainingReadBytes()", "remainingReadBytes must always be greater than 0!");
	
	return m_remainingReadBytes;
}


bool LocalSocketPrivate::writeDataRotated()
{
// 	qDebug("writeDataRotated()");
	
	bool	ret	=	false;
	
	switch(m_lastWrittenType)
	{
		case LAST_WRITTEN_NONE:
		case LAST_WRITTEN_PKG:
		{
			ret	=	writeSocketDescriptor() || ret;
			ret	=	writeData() || ret;
			ret	=	writePackageData() || ret;
		}break;
		
		case LAST_WRITTEN_DATA:
		{
			ret	=	writePackageData() || ret;
			ret	=	writeSocketDescriptor() || ret;
			ret	=	writeData() || ret;
		}break;
		
		case LAST_WRITTEN_SDESC:
		{
			ret	=	writeData() || ret;
			ret	=	writePackageData() || ret;
			ret	=	writeSocketDescriptor() || ret;
		}break;
	}
	
	m_lastWrittenType++;
	if(m_lastWrittenType > LAST_WRITTEN_MAX)
		m_lastWrittenType	=	LAST_WRITTEN_MIN;
	
	return ret;
}


bool LocalSocketPrivate::writeData()
{
	// Check and write pending data
	if(m_writeDataBuffer.isEmpty())
		return false;
	
// 	QByteArray		data(m_writeDataBuffer.peek(m_writeBufferSize - Header_size));
// 	QByteArray		data(QByteArray(Header_size, (char)0) + m_writeDataBuffer.dequeue(m_writeBufferSize - Header_size));
	QByteArray		dequeuedData(m_writeDataBuffer.dequeue(m_writeBufferSizeNet));
	quint32				totalSize	=	dequeuedData.size() + Header_size;
	
	// For debugging
	
// 	static quint64	totalWrittenLSPPeek	=	0;
// 	totalWrittenLSPPeek	+=	data.size() - Header_size;
// 	qDebug("total-written (LSP:dequeue): %llu", totalWrittenLSPPeek);
// 	Logger::log("Bytes dequeued (LocalSocketPrivate)", totalWrittenLSPPeek);
	
// 	if(data.size() <= Header_size)
// 		return false;
	
	// Copy data
	memcpy(m_writeBuffer.data() + Header_size, dequeuedData.constData(), dequeuedData.size());
	
	// Initialize header data
	struct Header	*	header	=	(Header*)m_writeBuffer.data();
	INIT_HEADER(*header, dequeuedData.size(), CMD_DATA, m_writeBuffer.constData() + Header_size);
	
	quint32	written	=	0;
	while(isOpen() && written < totalSize)
		written	+=	writeData(m_writeBuffer.constData() + written, totalSize - written);
	
// 	qDebug("so(Header): %d, written: %d, totalSize: %d, dSize: %d", Header_size, written, totalSize, dequeuedData.size());
	
	// Data has been written
	if(written == totalSize)
	{
		// For debugging
		
// 		static	quint64	totalWrittenLSUnix	=	0;
// 		static	quint64	totalWrittenLSUnix_net	=	0;
// 		totalWrittenLSUnix	+=	written + sizeof(header);
// 		totalWrittenLSUnix_net	+=	written;
// 		qDebug("total-written (ls): %llu (net: %llu) (available: %u)", totalWrittenLSUnix, totalWrittenLSUnix_net, m_writeBuffer.size());
// 		Logger::log("Bytes written (LocalSocketPrivate)", totalWrittenLSUnix);
// 		Logger::log("Bytes written net (LocalSocketPrivate)", totalWrittenLSUnix_net);
// 		Logger::log("Bytes available for writing (LocalSocketPrivate)", m_writeBuffer.size());
		
		emit(bytesWritten(totalSize));
		
		return true;
	}
	else
		return false;
	// Otherwise isOpen() returned false -> an error was already raised
}


bool LocalSocketPrivate::writePackageData()
{
	// Fetch current package to write
	if(m_currentWritePackage.size() - m_currentWritePackagePosition <= 0)
	{
		// Check and write pending data
		if(m_writePkgBuffer.isEmpty())
			return false;
		
		m_currentWritePackage					=	m_writePkgBuffer.dequeue();
		m_currentWritePackagePosition	=	0;
	}
	else
	{
		// Free some space if we have at least 10*writeBufferSize to free
		if(m_currentWritePackagePosition >= 10*m_writeBufferSize)
		{
			m_currentWritePackage					=		m_currentWritePackage.mid(10*m_writeBufferSize);
			m_currentWritePackagePosition	-=	10*m_writeBufferSize;
		}
	}
	
	/// Total remaining size of package
	quint32		size			=	m_currentWritePackage.size() - m_currentWritePackagePosition;
	/// Data portion size of this transfer
	quint32		dataSize	=	qMin(m_writeBufferSizeNet, size);
	quint32		totalSize	=	dataSize + Header_size;
	bool			endPkg		=	(dataSize == size);
	
	// Copy data
	memcpy(m_writeBuffer.data() + Header_size, m_currentWritePackage.constData() + m_currentWritePackagePosition, dataSize);
	
	// Initialize header data
	struct Header	*	header	=	(Header*)m_writeBuffer.data();
	INIT_HEADER((*header), dataSize, endPkg ? CMD_PKG_END : CMD_PKG, m_currentWritePackage.constData() + m_currentWritePackagePosition);
	
// 	// Add header and data
// 	QByteArray	data((const char*)&header, sizeof(header));
// 	data.append(m_currentWritePackage.constData() + m_currentWritePackagePosition, dataSize);
	
	quint32	written	=	0;
	
	// Write all data
	while(isOpen() && written < totalSize)
		written	+=	writeData(m_writeBuffer.constData() + written, totalSize - written);
	
	// Data has been written -> Remove from buffer
	if(written == totalSize)
	{
		if(endPkg)
		{
			m_currentWritePackagePosition	=	0;
			m_currentWritePackage.clear();
			
// 			static	int	totalPkgWritten	=	0;
// 			totalPkgWritten++;
// 			qDebug("totalPkgWritten: %d (toWrite: %d)", totalPkgWritten, m_writePkgBuffer.count());
			
			emit(packageWritten());
		}
		else
			m_currentWritePackagePosition	+=	dataSize;
		
		return true;
	}
	else
		return false;
	// Otherwise isOpen() returned false -> an error was already raised
}


bool LocalSocketPrivate::writeSocketDescriptor()
{
	if(m_writeSDescBuffer.isEmpty())
		return false;
		
	quintptr	sDesc	=	m_writeSDescBuffer.dequeue();
	
	// Don't write 0 socket descriptors
	if(!sDesc)
		return false;
	
// 	if(QCoreApplication::applicationFilePath().endsWith("external_peer"))
// 	{
// 		qFatal("!!! SENDING SOCKET !!!");
// 	}
	
	// Send an package with 1 byte whos data will be ignored
	quint32	totalSize	=	Header_size + 1;
	// Set the one byte
	*(m_writeBuffer.data() + Header_size)	=	0;
	
	// Initialize header data
	struct Header	*	header	=	(Header*)m_writeBuffer.data();
	INIT_HEADER((*header), totalSize - Header_size, CMD_SOCK, m_writeBuffer.constData() + Header_size);
	
	/*
	 * First write the package and afterwards the actual socket
	 * so the receiver can correctly notify the implementation that an socket is about to arrive
	 */
	
	// Write package
	quint32	writtenData	=	0;
	
	while(isOpen() && writtenData < totalSize)
		writtenData	+=	writeData(m_writeBuffer.constData() + writtenData, totalSize - writtenData);
	
	// Write socket descriptor
	bool	written	=	false;
	
	while(isOpen() && writtenData == totalSize && !written)
		written	=	writeSocketDescriptor(sDesc);
	
	if(written && writtenData == totalSize)
	{
		/*
		 * Don't emit socketDescriptorWritten() here as the socket might not be received by the peer yet
		 * and it would not be safe to close the socket descriptor at this time.
		 */
		
		return true;
	}
	else
		return false;
}


bool LocalSocketPrivate::isOpen() const
{
	return	m_isOpen;
}


int __LocalSocketPrivate_init()
{
	LocalSocketPrivateEvent::LocalSocketPrivateType	=	static_cast<QEvent::Type>(QEvent::registerEventType());
	
	qRegisterMetaType<LocalSocket::LocalSocketError>("LocalSocket::LocalSocketError");
	qRegisterMetaType<LocalSocket::LocalSocketState>("LocalSocket::LocalSocketState");
	qRegisterMetaType<quintptr>("quintptr");
	
	return 1;
}


Q_CONSTRUCTOR_FUNCTION(__LocalSocketPrivate_init);
