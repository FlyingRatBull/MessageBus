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

#ifndef LOCALSOCKETPRIVATE_H
#define LOCALSOCKETPRIVATE_H

#include <QtCore/QString>
#include <QtCore/QIODevice>
#include <QtCore/QEvent>

#include "localsocket.h"

#include "variant.h"
#include "tsqueue.h"
#include "tsdataqueue.h"
#include "tools.h"

class MSGBUS_LOCAL LocalSocketPrivateEvent : public QEvent
{
	public:
		static QEvent::Type LocalSocketPrivateType;
		
		enum Type
		{
			Connect,
			Disconnect,
			Flush,
			WriteData,
			WritePackage,
			WriteSocketDescriptor
		};
		
		LocalSocketPrivateEvent(Type type)
		: QEvent(LocalSocketPrivateType), socketEventType(type), socketDescriptorOpen(false)
		{
		}
		
		Type			socketEventType;
		
		// For connect
		Variant								connectTarget;
		QIODevice::OpenMode		connectMode;
		bool									socketDescriptorOpen;
};


class MSGBUS_LOCAL LocalSocketPrivate : public QObject
{
	Q_OBJECT

	public:
		TsDataQueue							m_readBuffer;
		TsQueue<QByteArray>			m_readPkgBuffer;
		TsQueue<int>						m_readSDescBuffer;
		
		
		LocalSocketPrivate(LocalSocket * q);
		
		virtual ~LocalSocketPrivate();
		
		void addToWriteBuffer(const char * data, int size);
		
		void addToPackageWriteBuffer(const QByteArray &package);
		
		void addToWriteBuffer(quintptr socketDescriptor);
		
		qint64 readBufferSize() const;
		
		void setReadBufferSize(qint64 size);
		
		qint64 writeBufferSize() const;
		
		qint64 writePackageBufferSize() const;
		
		qint64 writeSocketDescriptorBufferSize() const;
		
		bool canReadLine() const;
		
		QIODevice::OpenMode openMode() const;
		
		bool hasError() const;
		
		/*
		 * The following functions must be implemented by subclasses
		 */
		virtual quintptr socketDescriptor() const = 0;
		
	signals:
		void connected();
		
		void disconnected();
		
		void readyRead();
		
		void readyReadPackage();
		
		void readyReadSocketDescriptor();
		
		void bytesWritten(qint64 bytes);
		
		void packageWritten();
		
		void socketDescriptorWritten(quintptr socketDescriptor);
		
		void error(LocalSocket::LocalSocketError socketError, const QString& errorText);
		
	protected:
		bool event(QEvent * event);
		
		void setWriteBufferSize(quint32 size);
		
		void addReadData(const char *src, int size);
		
		void addReadSocketDescriptor(quintptr socketDescriptor, quintptr peerSocketDescriptor);
		
		void setOpened();
		
		void setClosed();
		
		bool isOpen() const;
		
		void setError(LocalSocket::LocalSocketError socketError, const QString& errorText);
		
		quint32 remainingReadBytes() const;
		
		/*
		 * The following functions must be implemented by subclasses
		 */
		virtual void open(quintptr socketDescriptor, bool socketOpen, QIODevice::OpenMode mode) = 0;
		
		virtual void open(const QString& name, QIODevice::OpenMode mode) = 0;
		
		virtual void close() = 0;
		
		/**
		 * @brief Writes data to the underlying socket.
		 * 
		 * @note This function may not block if it chooses to.
		 *
		 * @param data Pointer to data to write.
		 * @param size Size of data.
		 * @return Number of actually written bytes.
		 **/
		virtual quint32 writeData(const char * data, quint32 size) = 0;
		
		/**
		 * @brief Writes an socket descriptor to the underlying socket.
		 * 
		 * @note This function may either write the whole socket descriptor or nothing of it.
		 *
		 * @param socketDescriptor Socket descriptor to write.
		 * @return TRUE on success; FALSE otherweise.
		 **/
		virtual bool writeSocketDescriptor(quintptr socketDescriptor) = 0;
		
		/*
		 * The following functions are for convenience and need not to be implemented
		 */
		virtual void notifyReadSocketDescriptor() {};
		
		virtual void flush() {};
		
	private:
		bool writeDataRotated();
		
		bool writeData();
		
		bool writePackageData();
		
		bool writeSocketDescriptor();
		
	private:
		struct Header
		{
			quint32		cmd;
			quint32		size;
			quint16		crc16;
			bool			bigEndian;
		};
		// size of Header must be defined explicitely because the compiler can realign structs
	#define Header_size	(sizeof(quint32)*2 + sizeof(quint16) + sizeof(bool))
		
		bool										m_isOpen;
		QIODevice::OpenMode			m_openMode;
		bool										m_hasError;
		
		TsDataQueue							m_writeDataBuffer;
		TsQueue<QByteArray>			m_writePkgBuffer;
		TsQueue<int>						m_writeSDescBuffer;
		
		/// Buffer for outgoing data
		QByteArray							m_writeBuffer;
		
		/// Read buffer size
		qint64									m_readBufferSize;
		///Temporary read buffer (until one whole data portion is readable)
		QByteArray							m_tmpReadBuffer;
		/// Number of bytes to read to get full data portion
		quint32									m_remainingReadBytes;
		
		/// Size of implementation write buffer
		quint32									m_writeBufferSize;
		/// Size of implementation write buffer without header
		quint32									m_writeBufferSizeNet;
		
		/// Last written data type (Used for rotational write)
		uchar										m_lastWrittenType;
		/// Position into currently being written package
		uint										m_currentWritePackagePosition;
		QByteArray							m_currentWritePackage;
		
		QByteArray							m_currentReadPackage;
};

#endif // LOCALSOCKETPRIVATE_H
