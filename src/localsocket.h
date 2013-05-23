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

#ifndef LOCALSOCKET_H
#define LOCALSOCKET_H

#include <QtCore>
#include <QtNetwork>

#include "global.h"

class LocalSocketPrivate_Thread;

class LocalSocket : public QIODevice
{
	Q_OBJECT
	
	Q_ENUMS(LocalSocketError LocalSocketState)
	
	friend class LocalSocketPrivate_Thread;
	
	public:
		enum LocalSocketError
		{
			ConnectionRefusedError,
			PeerClosedError,
			ServerNotFoundError,
			SocketAccessError,
			SocketResourceError,
			SocketTimeoutError,
			DatagramTooLargeError,
			ConnectionError,
			UnsupportedSocketOperationError,
			UnknownSocketError
		};
		
		enum LocalSocketState
		{
			UnconnectedState,
			ConnectingState,
			ConnectedState,
			ClosingState
		};
	
	public:
		LocalSocket(QObject * parent = 0);
		
		virtual ~LocalSocket();
		
		virtual qint64 packagesAvailable() const;
		
		virtual qint64 socketDescriptorsAvailable() const;
		
		virtual QByteArray readPackage();
		
		virtual quintptr readSocketDescriptor();
		
		virtual bool writePackage(const QByteArray& package);
		
		virtual bool writeSocketDescriptor(quintptr socketDescriptor);
		
		virtual bool waitForPackageWritten(int msecs);
		
		virtual bool waitForSocketDescriptorWritten(int msecs);
		
		virtual qint64 packagesToWrite() const;
		
		virtual qint64 socketDescriptorsToWrite() const;
		
		virtual bool waitForReadyReadPackage(int msecs);
		
		virtual bool waitForReadyReadSocketDescriptor(int msecs);
		
		/*
		 * Functions from QLocalSocket
		 */
		void abort();
		
		void connectToServer(const QString& name, QIODevice::OpenMode mode);
		
		bool setSocketDescriptor(quintptr socketDescriptor, LocalSocketState socketState = ConnectedState, QIODevice::OpenMode openMode = QIODevice::ReadWrite);
		
		/**
		 * @brief Returns the native socket descriptor of the LocalSocket object if this is available; otherwise returns 0.
		 * 
		 * @note Unlike the QLocalSocket implementation class this function returns 0 if no socket is available because quintptr is unsigned!
		 *
		 * @return quintptr
		 **/
		quintptr socketDescriptor() const;
		
		void disconnectFromServer();
		
		LocalSocketError error() const;
		
		LocalSocketState state() const;
		
		bool flush();
		
		QString fullServerName() const;
		
		QString serverName() const;
		
		bool isValid() const;
		
		qint64 readBufferSize() const;
		
		void setReadBufferSize(qint64 size);
		
		qint64 writeBufferSize() const;
		
		void setWriteBufferSize(qint64 size);
		
		qint64 writePkgBufferSize() const;
		
		void setWritePkgBufferSize(qint64 size);
		
		bool waitForConnected(int msecs = 30000);
		
		bool waitForDisconnected(int msecs = 30000);
		
		/*
		 * Functions from QIODevice
		 */
		virtual bool canReadLine() const;
		
		virtual bool isSequential()
		{return true;}
		
		virtual void close()
		{abort();}
		
		virtual bool open(QIODevice::OpenMode openMode)
		{return state() == ConnectedState;}
		
		virtual qint64 bytesAvailable() const;
		
		virtual qint64 bytesToWrite() const;
		
		virtual bool waitForBytesWritten(int msecs);
		
		virtual bool waitForReadyRead(int msecs);
		
	signals:
		void readyReadPackage();
		
		void readyReadSocketDescriptor();
		
		void packageWritten();
		
		/**
		 * @brief An socket descriptor has been written
		 * 
		 * Unlike packageWritten() and bytesWritten() this signal gets only emitted when the file descriptor was received by the peer.
		 * So upon receiving this signal it is save to close the file descriptor.
		 *
		 * @param socketDescriptor Socket descriptor that was successfully written to the peer.
		 **/
		void socketDescriptorWritten(quintptr socketDescriptor);
		
		/*
		 * Signals from QLocalSocket
		 */
		void connected();
		
		void disconnected();
		
		void error(LocalSocket::LocalSocketError socketError);
		
		void stateChanged(LocalSocket::LocalSocketState socketState);
	
	protected:
		virtual qint64 readData(char *data, qint64 maxlen);
		
		virtual qint64 writeData(const char *data, qint64 len);
		
	private:
		LocalSocketPrivate_Thread		*	const	d_ptr;
};

#endif // LOCALSOCKET_H
