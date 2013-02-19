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


#ifndef TESTLOCALSOCKETPEERTHREAD_H
#define TESTLOCALSOCKETPEERTHREAD_H

#include <QThread>

#include <QLocalSocket>
#include <QReadWriteLock>
#include <QWaitCondition>

#include "../../localserver.h"


class TestLocalSocket_Peer;

class TestLocalSocketPeerThread : public QThread
{
	Q_OBJECT
	
	public:
		TestLocalSocketPeerThread(QObject * parent);
		
		virtual ~TestLocalSocketPeerThread();
		
		bool waitForStarted();

		TestLocalSocket_Peer * peer() const;

		LocalServer *localServer();
		
		bool waitForFinished(int timeout = 0);
		
	protected:
		virtual void run();
		
	private:
		mutable QReadWriteLock			m_peerLock;
		QWaitCondition						*	m_isPeerReady;
		TestLocalSocket_Peer			*	m_peer;
		
		LocalServer								*	m_localServer;
		
		// Running state
		bool												m_running;
		QReadWriteLock							m_runningLock;
		QWaitCondition							m_runningChanged;
};

#endif // TESTLOCALSOCKETPEERTHREAD_H
