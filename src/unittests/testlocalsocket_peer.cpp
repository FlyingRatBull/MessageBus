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

#include "testlocalsocket_peer.h"

TestLocalSocket_Peer::TestLocalSocket_Peer(): QThread()
{
	m_socket		=	0;
	rewriteData	=	false;
	
	start();
}


TestLocalSocket_Peer::~TestLocalSocket_Peer()
{
	if(m_socket)
		m_socket->deleteLater();
	
	if(isRunning())
	{
		terminate();
		wait();
	}
}


void TestLocalSocket_Peer::run()
{
	QThread::exec();
}


void TestLocalSocket_Peer::onNewConnection(quintptr socketDescriptor)
{
	QLocalServer	*	src	=	qobject_cast<QLocalServer*>(sender());
	
	if(src == 0)
		return;
	
	QVERIFY(m_socket == 0);
	
	m_socket	=	new LocalSocket(socketDescriptor);
	
	connect(m_socket, SIGNAL(readyRead()), SLOT(onReadyRead()), Qt::QueuedConnection);
	connect(m_socket, SIGNAL(readyReadPackage()), SLOT(onNewPackage()), Qt::QueuedConnection);
	
	QVERIFY(m_socket->open(QIODevice::ReadWrite));
}


void TestLocalSocket_Peer::onReadyRead()
{
	// Receive all data
	QCoreApplication::processEvents();
	
	QByteArray	data(m_socket->readAll());
	
	if(data.isEmpty())
		return;
	
// 	qDebug("TestLocalSocket_Peer: read %d bytes", data.size());
	
	receivedData.append(data);
	
	if(rewriteData)
	{
// 		qDebug("Rewriting %d bytes", data.size());
		
		qint64	written	=	0;
		int			i				=	0;
		
		while(written < data.size() && i++ < 50)
			written	+=	m_socket->write(data.constData() + written, data.size() - written);
	}
}


void TestLocalSocket_Peer::onNewPackage()
{
	QByteArray	data(m_socket->readPackage());
	receivedPackages.append(data);
		
	if(rewriteData)
	{
// 		qDebug("Rewriting package with %d bytes", data.size());
		
		if(!m_socket->writePackage(data))
			qDebug("Could not write package!");
	}
}
