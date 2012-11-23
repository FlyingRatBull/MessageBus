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

#ifndef TESTLOCALSOCKET_PEER_H
#define TESTLOCALSOCKET_PEER_H

#include <QtCore>
#include <QtNetwork>
#include <QtTest>

#include "../localsocket.h"


class TestLocalSocket_Peer : public QThread
{
	Q_OBJECT
	
	public:
		TestLocalSocket_Peer();
		
		virtual ~TestLocalSocket_Peer();
		
		// Test data
		QByteArray				orgData;
		QByteArray				receivedData;
		QList<QByteArray>	receivedPackages;
		
		bool						rewriteData;
		
		LocalSocket		*	m_socket;
		
	protected:
		virtual void run();
		
	public slots:
		void onNewConnection(quintptr socketDescriptor);
		
	private slots:
		void onReadyRead();
		
		void onNewPackage();
};

#endif // TESTLOCALSOCKET_PEER_H
