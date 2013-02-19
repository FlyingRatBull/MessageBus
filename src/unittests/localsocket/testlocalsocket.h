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

#ifndef TESTLOCALSOCKET_H
#define TESTLOCALSOCKET_H

#include <QtCore>
#include <QTest>

#include "../../localsocket.h"
#include "../../localserver.h"
#include "../../variant.h"

#include "testlocalsocket_peer.h"


class TestLocalSocketPeerThread;

class TestLocalSocket : public QObject
{
	Q_OBJECT
	
	public:
		TestLocalSocket();
		
	private slots:
		void init();
		
		void cleanup();
		
		// Tests
		void data();
		
		void package();
		
		void fileDescriptor();
		
		void random();
		
	private:
		void runTest(uchar dataAmnt, uchar pkgAmnt, uchar fdAmnt);
		
		Variant randomData(uchar *type, uchar dataAmnt, uchar pkgAmnt, uchar fdAmnt, QFile **targetFile) const;
		
		void closeFiles(const QList<QFile*>& files);
		
	private:
			LocalServer								*	m_localServer;
			LocalSocket								*	m_localSocket;
			
			TestLocalSocket_Peer			*	m_peer;
			TestLocalSocketPeerThread	*	m_peerThread;
			QLocalSocket							*	m_controlSocket;
			
			// List of open files
			QHash<QString, QFile*>			m_openFiles;
		
	public:
		static QString				s_socketId;
};

#endif // TESTLOCALSOCKET_H
