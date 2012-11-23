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

#include "testlocalsocket.h"

QString				TestLocalSocket::s_socketId	=	QString("TestLocalSocket");

TestLocalSocket::TestLocalSocket(): QObject()
{
	m_localServer	=	0;
	m_localSocket	=	0;
	
	m_peer	=	0;
	
	QThread::currentThread()->setObjectName("Thread: TestLocalSocket");
}


void TestLocalSocket::initTestCase()
{
	m_localServer	=	new LocalServer();
	QVERIFY(m_localServer->listen(s_socketId));
	
	// Init peer
	m_peer	=	new TestLocalSocket_Peer();
	
	connect(m_localServer, SIGNAL(newConnection(quintptr)), m_peer, SLOT(onNewConnection(quintptr)));
	
	m_localSocket	=	new LocalSocket(s_socketId);
	QVERIFY2(m_localSocket->open(QIODevice::ReadWrite), "Cannot connect to LocalServer");
	
	QCoreApplication::processEvents();
}


void TestLocalSocket::cleanupTestCase()
{
	if(m_localServer)
		m_localServer->deleteLater();
	if(m_localSocket)
		m_localSocket->deleteLater();
	if(m_peer)
		m_peer->deleteLater();
}


void TestLocalSocket::cleanup()
{
	// Test if data is still available (5 seconds)
// 	int	i	=	0;
// 	while(i++ < 50)
// 	{
// 		QTest::qWait(100);
// 		
// 		if(m_localSocket->bytesAvailable() || m_localSocket->packagesAvailable() || m_localSocket->socketDescriptorsAvailable())
// 			QFAIL("Still data available!");
// 		
// 		if(m_peer->m_socket->bytesAvailable() || m_peer->m_socket->packagesAvailable() || m_peer->m_socket->socketDescriptorsAvailable())
// 			QFAIL("Still data available!");
// 	}
}


void TestLocalSocket::sendData()
{
	QVERIFY(m_localSocket->isOpen());
	
// 	qDebug("Testing");
	QFETCH(QByteArray, data);
	
	m_peer->orgData			=	data;
	m_peer->rewriteData	=	false;
	m_peer->receivedData.reserve(data.size());
	
	qint64	written	=	0;
	int			i				=	0;
	
	while(written < data.size() && i++ < 50)
		written	+=	m_localSocket->write(data.constData() + written, data.size() - written);
	
	QVERIFY2(written == data.size(), "Not all data was written");
	
	i	=	0;
	while(m_peer->receivedData.size() < m_peer->receivedData.capacity() && i++ < 50)
		QTest::qWait(100);
	
	QVERIFY2(!m_peer->receivedData.isEmpty(), "Peer received no data");
	QCOMPARE(m_peer->receivedData.size(), data.size());
	QCOMPARE(m_peer->receivedData, data);
	
	m_peer->receivedData.clear();
	
	QCoreApplication::processEvents();
}


void TestLocalSocket::sendDataRewrite()
{
	QVERIFY(m_localSocket->isOpen());
	
	QFETCH(QByteArray, data);
// 	qDebug("Testing");
	
	m_peer->orgData			=	data;
	m_peer->rewriteData	=	true;
	m_peer->receivedData.reserve(data.size());
	
	qint64	written	=	0;
	int			i				=	0;
	
	while(written < data.size() && i++ < 50)
		written	+=	m_localSocket->write(data.constData() + written, data.size() - written);
	
	QVERIFY2(written == data.size(), "Not all data was written");
	
	i	=	0;
	while(m_peer->receivedData.size() < m_peer->receivedData.capacity() && i++ < 300)
		QTest::qWait(100);
	
	QVERIFY2(!m_peer->receivedData.isEmpty(), "Peer received no data");
	QCOMPARE(m_peer->receivedData.size(), data.size());
	QCOMPARE(m_peer->receivedData, data);
	
	m_peer->receivedData.clear();
	
	QCoreApplication::processEvents();
	
	// Wait for data to be rewritten
	i	=	0;
	while(m_localSocket->bytesAvailable() < data.size() && i++ < 300)
		QTest::qWait(100);

	QByteArray	recData(m_localSocket->readAll());
	
	QVERIFY2(!recData.isEmpty(), "Socket received no data");
	QCOMPARE(recData.size(), data.size());
	QCOMPARE(recData, data);
}


void TestLocalSocket::sendPackage()
{
	QVERIFY(m_localSocket->isOpen());
	
// 	qDebug("Testing");
	QFETCH(QByteArray, data);
	
	m_peer->orgData			=	data;
	m_peer->rewriteData	=	false;
	m_peer->receivedData.reserve(data.size());
	
	bool	written	=	m_localSocket->writePackage(data);
	
	QVERIFY(written);
	
	int	i	=	0;
	while(m_peer->receivedPackages.isEmpty() && i++ < 50)
		QTest::qWait(100);
	
	QVERIFY2(!m_peer->receivedPackages.isEmpty(), "Peer received no package");
	QCOMPARE(m_peer->receivedPackages.first().size(), data.size());
	QCOMPARE(m_peer->receivedPackages.first(), data);
	
	m_peer->receivedPackages.clear();
	
	QCoreApplication::processEvents();
}


void TestLocalSocket::sendPackageRewrite()
{
	QVERIFY(m_localSocket->isOpen());
	
// 	qDebug("Testing");
	QFETCH(QByteArray, data);
	
	m_peer->orgData			=	data;
	m_peer->rewriteData	=	true;
	m_peer->receivedData.reserve(data.size());
	
	bool	written	=	m_localSocket->writePackage(data);
	
	QVERIFY(written);
	
	int	i	=	0;
	while(m_peer->receivedPackages.isEmpty() && i++ < 50)
		QTest::qWait(100);
	
	QVERIFY2(!m_peer->receivedPackages.isEmpty(), "Peer received no package");
	QCOMPARE(m_peer->receivedPackages.first().size(), data.size());
	QCOMPARE(m_peer->receivedPackages.first(), data);
	
	m_peer->receivedPackages.clear();
	
	QCoreApplication::processEvents();
	
	// Wait for data to be rewritten
	i	=	0;
	while(m_localSocket->packagesAvailable() < 1 && i++ < 300)
		QTest::qWait(100);
	
	QByteArray	recData(m_localSocket->readPackage());
	
	QVERIFY2(!recData.isEmpty(), "Socket received no data");
	QCOMPARE(recData.size(), data.size());
	QCOMPARE(recData, data);
}


void TestLocalSocket::randomWithPeer()
{
	// Start peer
	QProcess		peer;
	QStringList	args;
	// Runtime for peer
	int					runTime	=	300;	// 5 min
	QTime				time;
	
	connect(&peer, SIGNAL(readyReadStandardOutput()), SLOT(readPeerOutput()));
	peer.start(QCoreApplication::applicationDirPath() + "test_localsocket_peer" + QCoreApplication::applicationFilePath().section("/", -1, -1).section("\\", -1, -1).section(".", -1, -1), args);
	
	QVERIFY(peer.waitForStarted());
	
	time.start();
	while(time.elapsed() < runTime * 1000)
	{
		QByteArray	command;
		
		int	action	=	qrand()/(double(RAND_MAX))*3;	// 0..2

		///@todo Move to extra file to reuse in peer
		switch(action)
		{
			// Send data
			case 0:
			{
				command.append(QString("d").toAscii());
			}break;
			
			// Send package
			case 1:
			{
				command.append(QString("p").toAscii());
			}break;
			
			// Send file descriptor
			case 2:
			{
				command.append(QString("s").toAscii());
			}break;
		}
		
		
	}
	
	QVERIFY(peer.waitForFinished());
}


void TestLocalSocket::sendData_data()
{
	QTest::addColumn<QByteArray>("data");
	
	qsrand((uint)this);
	
	for(int i = 0; i < 7; i++)
	{
		qint64	size	=	(qint64)qPow(10, i);
		QString	name	=	QString("%1 bytes").arg(size);
		
		QByteArray	data;
		data.reserve(size);
		
		for(qint64 j = 0; j < size; j += sizeof(int))
		{
			int	r	=	qrand();
// 			int	r	=	(j < 256 ? j : 255);
			data.append((char*)&r, qMin(size - data.size(), (qint64)sizeof(int)));
		}
		
		QTest::newRow(qPrintable(name))	<<	data;
	}
	
	// Random size
	for(int i = 0; i < 7; i++)
	{
		qint64	size	=	qrand()/1000;
		QString	name	=	QString("%1 bytes").arg(size);
		
		QByteArray	data;
		data.reserve(size);
		
		for(qint64 j = 0; j < size; j += sizeof(int))
		{
			int	r	=	qrand();
			data.append((char*)&r, qMin(size - data.size(), (qint64)sizeof(int)));
		}
		
		QTest::newRow(qPrintable(name))	<<	data;
	}
}


void TestLocalSocket::sendDataRewrite_data()
{
	// Use data from sendData()
	sendData_data();
}


void TestLocalSocket::sendPackage_data()
{
	// Use data from sendData()
	sendData_data();
}


void TestLocalSocket::sendPackageRewrite_data()
{
	// Use data from sendData()
	sendData_data();
}

QTEST_MAIN(TestLocalSocket)
