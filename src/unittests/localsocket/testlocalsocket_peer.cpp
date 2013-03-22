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

#include "testlocalsocket_peer.h"

#include "../../localserver.h"
#include "../../tools.h"
#include "../logger.h"

TestLocalSocket_Peer::TestLocalSocket_Peer()
	:	QObject(), m_extProcess(0), m_socket(0), m_localCtrlServer(0), m_peerSocket(0), m_recSuccessCount(0), m_recDisconnectedSignal(false)
{
}


TestLocalSocket_Peer::~TestLocalSocket_Peer()
{
	{
		QWriteLocker		procLocker(&m_extProcessLock);
		
		if(m_extProcess)
		{
			m_extProcess->close();
			m_extProcess->deleteLater();
		}
		
		m_extProcess	=	0;
		m_extProcessReady.wakeAll();
	}
	
	{
		QWriteLocker	socketLocker(&m_socketLock);
		
		if(m_socket)
			m_socket->deleteLater();
		
		m_socket	=	0;
		m_socketChanged.wakeAll();
	}
	
	if(m_peerSocket)
	{
		QWriteLocker		locker(&m_peerSocketLock);
		
		m_peerSocket->abort();
		m_peerSocket->deleteLater();
		m_peerSocket	=	0;
		
		m_peerSocketReady.wakeAll();
	}
	
	{	
		QWriteLocker	ctrlLocker(&m_peerSocketLock);
		
		if(m_localCtrlServer)
		{
			m_localCtrlServer->close();
			m_localCtrlServer->deleteLater();
		}
		
		m_localCtrlServer	=	0;
		m_peerSocketReady.wakeAll();
	}
}


void TestLocalSocket_Peer::startExternalPeer(const QString &socketId)
{
	// Create QLocalServer
	m_localCtrlServer	=	new QLocalServer();
	
	QLocalServer::removeServer("TestLocalSocket_Peer");
	if(!m_localCtrlServer->listen("TestLocalSocket_Peer"))
		qFatal("Could not listen on QLocalServer!");
	
	QWriteLocker		locker(&m_extProcessLock);
	m_extProcess	=	new QProcess(this);
	
	// Forward output
	m_extProcess->setProcessChannelMode(QProcess::ForwardedChannels);
	
	// Start source
	QStringList	args;
	args.append(socketId);
	
//  #define USE_VALGRIND
	
#ifdef USE_VALGRIND
	args.prepend(QCoreApplication::applicationFilePath().replace("test_localsocket", "test_localsocket_external_peer"));
	
	// --tool=memcheck -v -v --leak-check=full --show-reachable=yes
	args.prepend("--tool=memcheck");
	args.prepend("--log-file=/tmp/testlocalsocket_peer.valgrind.out");
	args.prepend("-v");
	args.prepend("-v");
	args.prepend("--leak-check=full");
// 	args.prepend("--show-reachable=yes");
	
	m_extProcess->start("valgrind", args);
#else
	m_extProcess->start(QCoreApplication::applicationFilePath().replace("test_localsocket", "test_localsocket_external_peer"), args);
#endif
	
	if(!m_extProcess->waitForStarted(60000))
	{
		m_extProcess->terminate();
		qFatal("Could not start external peer!");
		
		return;
	}
	
	m_extProcessReady.wakeAll();
	
	callSlotAuto(this, "fetchControlSocket");
}


bool TestLocalSocket_Peer::waitForExternalPeer(int timeout)
{
	QReadLocker		locker(&m_extProcessLock);
	if(m_extProcess == 0)
		m_extProcessReady.wait(&m_extProcessLock, timeout);
	
	return (m_extProcess != 0);
}


bool TestLocalSocket_Peer::waitForExternalSocket(int timeout)
{
	QReadLocker		locker(&m_socketLock);
	if(m_socket == 0)
		m_socketChanged.wait(&m_socketLock, timeout);
	
	return (m_socket != 0);
}


bool TestLocalSocket_Peer::waitForControlSocket(int timeout)
{
	QReadLocker		locker(&m_peerSocketLock);
	
	if(m_peerSocket)
		return m_peerSocket;
	
	m_peerSocketReady.wait(&m_peerSocketLock, timeout);
	
	return	(m_peerSocket != 0);
}


bool TestLocalSocket_Peer::waitForTotalSuccessCount(quint64 numSuccess, int stepTimeout)
{
	QReadLocker		locker(&m_recLock);
	
	if(!m_recFailureMessages.isEmpty())
		return false;
	
	if(m_recSuccessCount >= numSuccess)
		return true;
	
	QElapsedTimer	timer;
	timer.start();
	while(m_recSuccessCount < numSuccess)
	{
		QCoreApplication::processEvents();
		// Return if we did not receive data or if we received an failure
		timer.restart();
		
		quint64	old	=	m_recSuccessCount;
		if(!m_recValuesChanged.wait(&m_recLock, stepTimeout) || !m_recFailureMessages.isEmpty())
		{
			if(old == m_recSuccessCount)
			{
				if(timer.elapsed() >= stepTimeout)
					break;
				else
					QCoreApplication::processEvents();
			}
		}
	}
	
	return (!m_recFailureMessages.isEmpty() && m_recSuccessCount >= numSuccess);
}


bool TestLocalSocket_Peer::waitForDisconnectedSignal(int timeout)
{
	QReadLocker		readLocker(&m_recDiscLock);
	
	if(m_recDisconnectedSignal)
		return true;
	
	return m_recDiscSignalChanged.wait(&m_recDiscLock, timeout);
}


bool TestLocalSocket_Peer::writeControlData(const QByteArray &data)
{
	m_writeControlData.enqueue(data);
	
	if(!callSlotAuto(this, "writeControlData"))
		qFatal("Auto slot call of \"writeControlData\" failed!");
	
	return true;
}


void TestLocalSocket_Peer::stopProcess()
{
	if(!m_extProcess || !m_extProcess->isOpen())
		return;
	
	m_extProcess->terminate();
}


quint64 TestLocalSocket_Peer::totalSuccessCount() const
{
	QReadLocker		locker(&m_recLock);
	
	return m_recSuccessCount;
}


QList< quintptr > TestLocalSocket_Peer::fileDescriptorSignals() const
{
	QReadLocker		locker(&m_recFileDescSignalsLock);
	
	return m_recFileDescSignals;
}


QStringList TestLocalSocket_Peer::failureMessages() const
{
	QReadLocker		locker(&m_recLock);
	
	return m_recFailureMessages;
}


bool TestLocalSocket_Peer::receivedFailure() const
{
	QReadLocker		locker(&m_recLock);
	
	return !m_recFailureMessages.isEmpty();
}


QLocalSocket *TestLocalSocket_Peer::controlSocket() const
{
	QReadLocker		locker(&m_peerSocketLock);
	return m_peerSocket;
}


LocalSocket *TestLocalSocket_Peer::externalSocket() const
{
	QReadLocker		locker(&m_socketLock);
	return m_socket;
}


void TestLocalSocket_Peer::onNewConnection(quintptr socketDescriptor)
{
	LocalServer	*	src	=	qobject_cast<LocalServer*>(sender());
	
	if(src == 0)
		return;
	
	QWriteLocker	locker(&m_socketLock);
	
	QVERIFY(m_socket == 0);
	
	m_socket	=	new LocalSocket(this);
	
	connect(m_socket, SIGNAL(socketDescriptorWritten(quintptr)), SLOT(fileDescriptorWritten(quintptr)));
	connect(m_socket, SIGNAL(disconnected()), SLOT(onDisconnected()), Qt::DirectConnection);
	
	m_socket->setSocketDescriptor(socketDescriptor);
	
	QVERIFY(m_socket->open(QIODevice::ReadWrite));
	
	m_socketChanged.wakeAll();
}


void TestLocalSocket_Peer::fetchControlSocket()
{
	QWriteLocker		locker(&m_peerSocketLock);
	
	if(m_peerSocket)
	{
		m_peerSocketReady.wakeAll();
		return;
	}
	
	QLocalSocket	*	ret	=	m_localCtrlServer->nextPendingConnection();
	
	QElapsedTimer		timer;
	timer.start();
	
	while(!ret && timer.elapsed() < 30000)
	{
		m_localCtrlServer->waitForNewConnection(2000);
		ret	=	m_localCtrlServer->nextPendingConnection();
	}
	
	m_peerSocket	=	ret;
	
	connect(m_peerSocket, SIGNAL(readyRead()), SLOT(readControlData()));
	
// 	qDebug("Got control socket!");
	m_peerSocketReady.wakeAll();
}


void TestLocalSocket_Peer::readControlData()
{
	QVERIFY(m_peerSocket->isOpen() && m_peerSocket->openMode() & QIODevice::ReadOnly);
	
	if(!m_peerSocket->canReadLine())
		return;
	
// 	qDebug("Got: %lld (canReadLine: %s)", m_peerSocket->bytesAvailable(), m_peerSocket->canReadLine() ? "true" : "false");
	
	while(m_peerSocket->canReadLine())
	{
// 		qDebug("Reading...");
		QString	line(m_peerSocket->readLine());
// 		qDebug("Read: %s", qPrintable(line));
		
		if(line.isEmpty())
			continue;
		
		static	int	retCount	=	0;
		retCount++;
		Logger::log("Handled returns (TestLocalSocket)", retCount);
		
		QWriteLocker		locker(&m_recLock);
		
		switch(line.at(0).toAscii())
		{
			// Failure
			case 'f':
			{
				m_recFailureMessages.append(line.mid(1));
			}break;
			
			// Success
			case 's':
			{
				m_recSuccessCount++;
			}break;
		}
		
		m_recValuesChanged.wakeAll();
		
		QCoreApplication::processEvents();
	}
}


void TestLocalSocket_Peer::writeControlData()
{
// 	QByteArray		data(m_writeControlData.dequeue(8192));		///@bug Calls qFree/.../abort() on destruction
	QByteArray		data;
	int						timeout	=	30000;
	
	// Workaround for double free bug of ~QByteArray
	data.resize(8192);
	uint	dequeued	=	m_writeControlData.dequeue(data.data(), 8192);
	data.resize(dequeued);
	
// 	qDebug("writeControlData");
	
	QMutexLocker		locker(&m_peerWriteMutex);
	
	if(!(m_peerSocket->isOpen() && m_peerSocket->openMode() & QIODevice::WriteOnly))
	{
		qFatal("Control socket closed when trying to write data!");
		return;
	}
	
	qint64					written	=	0;
	qint64					tmp			=	0;
	QElapsedTimer		timer;
	
	timer.start();
	
	do
	{
		tmp	=	m_peerSocket->write(data.constData() + written, data.size() - written);
		
		if(tmp < 0)
		{
			qFatal("Could not write control data: %s", qPrintable(m_peerSocket->errorString()));
			return;
		}
		
		written	+=	tmp;
	}while(written < data.size() && timer.elapsed() < timeout);
	
// 	qDebug("writeControlData = true");
	
	if(written < data.size())
		qFatal("Timeout when trying to write control data!");
	
	if(!m_writeControlData.isEmpty())
		callSlotQueued(this, "writeControlData");
}


void TestLocalSocket_Peer::fileDescriptorWritten(quintptr fd)
{
	QWriteLocker		locker(&m_recFileDescSignalsLock);
	
	m_recFileDescSignals.append(fd);
}


void TestLocalSocket_Peer::onDisconnected()
{
	QWriteLocker		readLocker(&m_recDiscLock);
	
	m_recDisconnectedSignal	=	true;
	m_recDiscSignalChanged.wakeAll();
}
