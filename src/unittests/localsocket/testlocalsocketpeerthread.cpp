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


#include "testlocalsocketpeerthread.h"

#include <QLocalServer>

#include "testlocalsocket.h"
#include "testlocalsocket_peer.h"
#include "../../tools.h"


TestLocalSocketPeerThread::TestLocalSocketPeerThread(QObject * parent)
:	QThread(parent), m_peer(0), m_localServer(0), m_isPeerReady(0), m_running(false)
{
	setObjectName("TestLocalSocketPeerThread");
	
	m_isPeerReady	=	new QWaitCondition();
}


TestLocalSocketPeerThread::~TestLocalSocketPeerThread()
{
	if(QThread::isRunning())
		QThread::quit();
	
	waitForFinished();
	
	{
		QWriteLocker		peerLocker(&m_peerLock);
		m_isPeerReady->wakeAll();
	}
	// Release lock so everyone waiting on it can continue before deletion
	{
		QWriteLocker		peerLocker(&m_peerLock);
		delete m_isPeerReady;
		m_isPeerReady	=	0;
	}
}


bool TestLocalSocketPeerThread::waitForStarted()
{
	QReadLocker		peerLocker(&m_peerLock);
	
	if(m_peer == 0 && m_isPeerReady != 0)
		m_isPeerReady->wait(&m_peerLock, 30000);
	
	return (m_peer != 0);
}


TestLocalSocket_Peer * TestLocalSocketPeerThread::peer() const
{
	QReadLocker		peerLocker(&m_peerLock);
	
	return m_peer;
}


LocalServer * TestLocalSocketPeerThread::localServer()
{
	return m_localServer;
}


bool TestLocalSocketPeerThread::waitForFinished(int timeout)
{
	QReadLocker		runningLocker(&m_runningLock);
	
	if(m_running)
		m_runningChanged.wait(&m_runningLock, timeout);
	
	return (!m_running);
}


void TestLocalSocketPeerThread::run()
{
	{
		QWriteLocker		runningLocker(&m_runningLock);
		m_running	=	true;
		m_runningChanged.wakeAll();
	}
	
	// Start LocalServer
	LocalServer		localServer;
	
	m_localServer	=	&localServer;
	
	if(!localServer.listen(TestLocalSocket::s_socketId))
		qFatal("Could not listen on LocalServer!");
	
	
	m_peerLock.lockForWrite();
	m_peer	=	new TestLocalSocket_Peer();
	if(m_isPeerReady)
		m_isPeerReady->wakeAll();
	m_peerLock.unlock();
	
	QThread::exec();
	
	m_peerLock.lockForWrite();
	delete m_peer;
	m_peer	=	0;
	m_peerLock.unlock();
	
	QLocalServer::removeServer("TestLocalSocket_Peer");
	
	{
		QWriteLocker		runningLocker(&m_runningLock);
		m_running	=	false;
		m_runningChanged.wakeAll();
	}
}

