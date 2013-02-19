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

#ifndef TESTLOCALSOCKET_PEER_H
#define TESTLOCALSOCKET_PEER_H

#include <QtCore>
#include <QtNetwork>
#include <QtTest>

#include "../../localsocket.h"
#include "../../tsdataqueue.h"


class TestLocalSocket_Peer : public QObject
{
	Q_OBJECT
	
	public:
		TestLocalSocket_Peer();
		
		virtual ~TestLocalSocket_Peer();
		
		bool waitForExternalPeer(int timeout = 0);
		
		bool waitForExternalSocket(int timeout = 0);
		
		bool waitForControlSocket(int timeout = 0);
		
		bool waitForTotalSuccessCount(quint64 numSuccess, int stepTimeout = 0);
		
		bool writeControlData(const QByteArray &data);
		
		quint64 totalSuccessCount() const;
		
		QList<quintptr> fileDescriptorSignals() const;
		
		QStringList failureMessages() const;
		
		bool receivedFailure() const;
		
		QLocalSocket * controlSocket() const;
		
		LocalSocket * externalSocket() const;
		
	public slots:
		void startExternalPeer(const QString &socketId);
		
		void onNewConnection(quintptr socketDescriptor);
		
	private slots:
		void fetchControlSocket();
		
		void readControlData();
		
		void writeControlData();
		
		void fileDescriptorWritten(quintptr fd);
		
	private:
		// External peer process
		QProcess									*	m_extProcess;
		mutable	QReadWriteLock			m_extProcessLock;
		QWaitCondition							m_extProcessReady;
		
		// LocalSocket to external process (data)
		LocalSocket								*	m_socket;
		mutable	QReadWriteLock			m_socketLock;
		QWaitCondition							m_socketChanged;
		
		// QLocalServer
		QLocalServer							*	m_localCtrlServer;
		// QLocalSocket to external process (ctrl)
		QLocalSocket							*	m_peerSocket;
		mutable	QReadWriteLock			m_peerSocketLock;
		QWaitCondition							m_peerSocketReady;
		QMutex											m_peerWriteMutex;
		
		// Control data to be written
		TsDataQueue									m_writeControlData;
		
		// Received signals for written file descriptors
		QList<quintptr>							m_recFileDescSignals;
		mutable	QReadWriteLock			m_recFileDescSignalsLock;
		
		// Received control values
		quint64											m_recSuccessCount;
		QStringList									m_recFailureMessages;
		mutable	QReadWriteLock			m_recLock;
		QWaitCondition							m_recValuesChanged;
};

#endif // TESTLOCALSOCKET_PEER_H
