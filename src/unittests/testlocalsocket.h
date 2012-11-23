// Copyright 2012 Oliver Becker <der.ole.becker@gmail.com>
// 
// This file is part of the MessageBus project,
// 
// MessageBus is free software: you can redistribute it and/or modify it
// under the terms of the GNU General Public License as published by the Free Software Foundation,
// either version 3 of the License, or (at your option) any later version.
// MessageBus is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
// without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
// See the GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License along with MessageBus.
// If not, see http://www.gnu.org/licenses/.

#ifndef TESTLOCALSOCKET_H
#define TESTLOCALSOCKET_H

#include <QtCore>
#include <QTest>

#include "../localsocket.h"
#include "../localserver.h"

#include "testlocalsocket_peer.h"


class TestLocalSocket : public QObject
{
	Q_OBJECT
	
	public:
		TestLocalSocket();
	
	private slots:
		void initTestCase();
		
		void cleanupTestCase();
		
		void cleanup();
		
		// Tests
		void sendData();
		
		void sendDataRewrite();
		
		void sendPackage();
		
		void sendPackageRewrite();
		
		void randomWithPeer();
		
		// Test data
		void sendData_data();
		
		void sendDataRewrite_data();
		
		void sendPackage_data();
		
		void sendPackageRewrite_data();
		
	private:
			LocalServer				*	m_localServer;
			LocalSocket				*	m_localSocket;
			
			TestLocalSocket_Peer		*	m_peer;
		
	private:
		static QString				s_socketId;
};

#endif // TESTLOCALSOCKET_H
