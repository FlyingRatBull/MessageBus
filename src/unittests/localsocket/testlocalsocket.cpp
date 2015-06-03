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

#include "testlocalsocket.h"

#include <signal.h>

#include "testlocalsocketpeerthread.h"
#include "../logger.h"
#include "../../tools.h"
#include "../../variant.h"

QString				TestLocalSocket::s_socketId	=	QStringLiteral("TestLocalSocket");


TestLocalSocket::TestLocalSocket()
	:	QObject(), m_localServer(0), m_localSocket(0), m_peer(0), m_peerThread(0), m_controlSocket(0)
{
	QThread::currentThread()->setObjectName("Thread: TestLocalSocket");
	
	// QLocalSocket throws SIGPIE
	signal(SIGPIPE, SIG_IGN);
	
	// Set log file
	QFile::remove("/tmp/testlocalsocket.log");
	Logger::setLogFile("/tmp/testlocalsocket.log");
	// Open log viewer
// 	QProcess::startDetached("/usr/local/bin/QLogVisualizer", QStringList() << Logger::logFile());
}


void TestLocalSocket::init()
{
	QVERIFY(m_localServer == 0);
	QVERIFY(m_localSocket == 0);
	QVERIFY(m_peer == 0);
	QVERIFY(m_peerThread == 0);
	QVERIFY(m_controlSocket == 0);
	
	// Init peer thread
	m_peerThread	=	new TestLocalSocketPeerThread(this);
	m_peerThread->start();
	
	QVERIFY(m_peerThread->waitForStarted());
	
	// Init local server
	m_localServer	=	m_peerThread->localServer();
	QVERIFY(m_localServer != 0);
	
	// Init peer
	m_peer	=	m_peerThread->peer();
	
	connect(m_localServer, SIGNAL(newConnection(quintptr)), m_peer, SLOT(onNewConnection(quintptr)), Qt::DirectConnection);
	
	// Start and wait for external peer
	callSlotAuto(m_peer, "startExternalPeer", Q_ARG(QString, s_socketId));
	QVERIFY(m_peer->waitForExternalPeer(30000));
	
	// Wait for control socket
	QVERIFY(m_peer->waitForControlSocket(30000));
	m_controlSocket	=	m_peer->controlSocket();
	
	QVERIFY(m_controlSocket != 0);
	
	// Wait for local socket to be ready
	QVERIFY(m_peer->waitForExternalSocket(30000));
	
	m_localSocket	=	m_peer->externalSocket();
	
	QVERIFY(m_localSocket != 0);
}


void TestLocalSocket::cleanup()
{
	// Close and delete all files
	foreach(const QString& filename, m_openFiles.keys())
	{
		m_openFiles[filename]->close();
		delete m_openFiles[filename];
	}
	m_openFiles.clear();
	
	// LocalServer gets deleted by its thread
	m_localServer	=	0;
	
	// LocalSocket gets deleted by peer class
	m_localSocket	=	0;
	
	// QLocalSocket gets deleted by peer class
	m_controlSocket	=	0;
	
	if(m_peerThread)
	{
		// Stop external process
		m_peerThread->peer()->stopProcess();
		
		// Wait for disconnected signal
		QVERIFY(m_peerThread->peer()->waitForDisconnectedSignal(5000));
		
		/// Thread doesn't exit
		m_peerThread->quit();
		
		if(!m_peerThread->waitForFinished(10000))
			m_peerThread->terminate();
		
 		m_peerThread->deleteLater();
		m_peerThread	=	0;
	}
	
	m_peer	=	0;
}


void TestLocalSocket::data()
{
	runTest(1, 0, 0);
}


void TestLocalSocket::package()
{
	runTest(0, 1, 0);
}


void TestLocalSocket::fileDescriptor()
{
	runTest(0, 0, 1);
}


void TestLocalSocket::random()
{
	runTest(1, 1, 1);
}


void TestLocalSocket::runTest(uchar dataAmnt, uchar pkgAmnt, uchar fdAmnt)
{
	quint64					sendCount					=	0;
	quint64					sendCountPackage	=	0;
	quint64					sendCountData			=	0;
	quint64					sendCountFD				=	0;
	quint64					sendBytes					=	0;
	
	// Runtime for peer in seconds
	int							runTime	=	15;	// In seconds
	QElapsedTimer		time;
	
	// Log timer
	QElapsedTimer		logTimer;
	const	int				logInterval	=	200;	// In msec
	logTimer.start();
	
	printf("\tRunning %d seconds\n", runTime);
	
	time.start();
	while(m_localSocket && time.elapsed() < runTime * 1000)
	{
		QVERIFY2(m_localSocket->isOpen(), qPrintable(QStringLiteral("LocalSocket not open! (" + m_localSocket->errorString() + ")")));
		
		QChar				command;
		uchar				action;
		QFile			*	file	=	0;
		QString			filename;
		
		Variant			data(randomData(&action, dataAmnt, pkgAmnt, fdAmnt, &file));
		
		// data can be invalid if we only test file descriptors as they have an limit
		if(dataAmnt == 0 && pkgAmnt == 0 && !data.isValid())
			break;
		
		// randomData returned invalid data -> Error already printed
		QVERIFY(data.isValid());
		
		// Handle filename
		if(file != 0)
		{
			filename	=	QFileInfo(*file).absoluteFilePath();
			m_openFiles[filename]	=	file;
		}
		
		// 		if(!m_peer->m_socket->isOpen() && !m_peer->m_socket->bytesAvailable())
		// 		{
			// 			returnLock.lockForWrite();
			// 			failureReason		=	"Socket closed";
			// 			failureReceived	=	true;
			// 			returnLock.unlock();
			// 		}
			// 		qDebug("Open: %s", m_peer->m_socket->isOpen() ? "true" : "false");
			
			switch(action)
			{
				// Send data
				case 0:
				{
					QByteArray		dataAr(data.toByteArray());
					QByteArray		hash(QCryptographicHash::hash(dataAr, QCryptographicHash::Md5));
					command	=	'd';
					
					// 				qDebug("Sending data with size %d", dataAr.size());
					
					QVERIFY2(m_peer->writeControlData(QString(command).toLatin1() + hash.toHex() + "\n"), "Could not write data to peer!");
					
					// size
					quint32	size	=	qToBigEndian<quint32>(dataAr.size());
					// 				qDebug("Size written: %d", data.size());
					dataAr.prepend((const char*)&size, sizeof(size));
					
					// 				static quint64	totalWrittenTest	=	0;
					// 				totalWrittenTest	+=	data.size();
					// 				qDebug("total-written (test): %llu", totalWrittenTest);
					
					qint64	written	=	0;
					QElapsedTimer	timer;
					timer.start();
					
					while(m_localSocket && m_localSocket->isOpen() && written < dataAr.size() && timer.elapsed() < 30000)
					{
						qint64	tmp	=	m_localSocket->write(dataAr.constData() + written, dataAr.size() - written);
						
// 						if(tmp < 0)
// 							qDebug("tmp < 0! open: %s", m_localSocket->isOpen() ? "true" : "false");
						QVERIFY2(tmp >= 0, qPrintable(QStringLiteral("Could not write all data to peer! (" + m_localSocket->errorString() + ")")));
						
						written	+=	tmp;
// 						QCoreApplication::processEvents();
						m_localSocket->waitForBytesWritten(3000);
					}
					
					// For debugging
					
					// 				printf("\t=== Data written ===\n");
					// 				for(int i = 0; i < data.size(); i++)
					// 					printf("\t%02X", data.constData()[i]);
					// 				printf("\t\n====================\n");
					
					QVERIFY2(written == dataAr.size(), "Could not write all data to peer!");
					
					sendCountData++;
					sendBytes	+=	dataAr.size() - sizeof(size);
					
					dataAr.clear();
					
					// 				qDebug("Sent data!");
				}break;
				
				// Send package
				case 1:
				{
					QByteArray		dataAr(data.toByteArray());
					QByteArray		hash(QCryptographicHash::hash(dataAr, QCryptographicHash::Md5));
					command	=	'p';
					
					QVERIFY2(m_peer->writeControlData(QString(command).toLatin1() + hash.toHex() + "\n"), "Could not write data to peer!");
					
					bool result = m_localSocket->writePackage(dataAr);
					
					while(!result && m_localSocket->isOpen())
					{
						m_localSocket->waitForPackageWritten(30000);
						result = m_localSocket->writePackage(dataAr);
					}
						
					QVERIFY2(result, "Could not write package!");
					
					sendCountPackage++;
					sendBytes	+=	dataAr.size();
					
					dataAr.clear();
					
					// 				qDebug("Sent package!");
				}break;
				
				// Send file descriptor
				case 2:
				{
					QByteArray		dataAr(filename.toLatin1());
					
					command	=	's';
					
					QVERIFY2(m_peer->writeControlData(QString(command).toLatin1() + dataAr.toHex() + "\n"), "Could not write data to peer!");
					QVERIFY2(m_localSocket->writeSocketDescriptor(data.toSocketDescriptor()), "Could not write socket descriptor!");
					
					sendCountFD++;
					
					// 				qDebug("Sent socket descriptor!");
				}break;
			}
			
			QCoreApplication::processEvents();
			
			// 		qDebug("Sent data!");
			
			sendCount++;
			
			if(logTimer.elapsed() >= logInterval)
			{
				Logger::log("Written commands (TestLocalSocket)", sendCount);
				logTimer.restart();
			}
			
			QStringList	failures(m_peer->failureMessages());
			QVERIFY2(failures.isEmpty(), qPrintable(QStringLiteral("Received failures:\n\t- %1").arg(failures.join("\n\t- "))));
	}
	
	Logger::log("Written commands (TestLocalSocket)", sendCount, "wB", "Waiting for bytes to be written");
	
	printf("\tSent %llu actions (%llu data portions, %llu packages, %llu file descriptors)\n", sendCount, sendCountData, sendCountPackage, sendCountFD);
	
	quint64			sendBytesFormatted	=	sendBytes;
	QStringList	formatter;
	formatter	<< "b" << "KB" << "MB" << "GB" << "TB" << "PB";
	int	idx	=	0;
	while(sendBytesFormatted > 1024*10)
	{
		sendBytesFormatted	/=	1024;
		idx++;
	}
	printf("\tSent %llu bytes (%llu %s)\n", sendBytes, sendBytesFormatted, qPrintable(formatter[idx]));
	
	printf("\tWaiting for results (This could take a while)\n");
	
	while(m_localSocket && m_localSocket->isOpen() && m_localSocket->bytesToWrite())
	{
		QCoreApplication::processEvents();
		m_localSocket->waitForBytesWritten(10000);
	}
	
	// Send close command
	// 	qDebug("Sending close command!");
	Logger::log("Written commands (TestLocalSocket)", sendCount, "sC", "Sent close command");
	QVERIFY2(m_peer->writeControlData(QStringLiteral("c\n").toLatin1()), "Could not write data to peer!");
	
	QCoreApplication::processEvents();
	
	if(!m_peer->waitForTotalSuccessCount(sendCount, 20000))
	{
		QStringList	failures(m_peer->failureMessages());
		QVERIFY2(failures.isEmpty(), qPrintable(QStringLiteral("Received failures:\n\t- %1").arg(failures.join("\n\t- "))));
		
		if(m_peer->totalSuccessCount() != sendCount)
			Logger::log("Written commands (TestLocalSocket)", sendCount, "f", "Failed to wait for all return commands");
		
		QVERIFY2(m_peer->totalSuccessCount() == sendCount, qPrintable(QStringLiteral("Invalid return count. Expected: %1; Got: %2").arg(sendCount).arg(m_peer->totalSuccessCount())));
	}
	
	// Check signal socketDescriptorWritten()
	QList<quintptr>	neededFdSignals;
	foreach(QFile * file, m_openFiles.values())
		neededFdSignals.append((quintptr)file->handle());
	
	QCoreApplication::processEvents();
	
	QList<quintptr>	emittedFdSignals(m_peer->fileDescriptorSignals());
	
	for(int i = 0; i < emittedFdSignals.count(); i++)
		QVERIFY2(neededFdSignals.removeOne(emittedFdSignals[i]), qPrintable(QStringLiteral("Invalid socketDescriptorWritten() signal")));
	
	QVERIFY2(neededFdSignals.isEmpty(), qPrintable(QStringLiteral("Missing socketDescriptorWritten() signals: %1").arg(neededFdSignals.count())));
	
	
	Logger::log("Written commands (TestLocalSocket)", sendCount, "d", "Done");
}


Variant TestLocalSocket::randomData(uchar *type, uchar dataAmnt, uchar pkgAmnt, uchar fdAmnt, QFile **targetFile) const
{
	///@todo Get actual user file number limit and use it here
	const int		maxFiles	=	512;	// Limit number of files to send
	
	if(m_openFiles.count() >= maxFiles)
		fdAmnt	=	0;
		
	ushort	maxAmnt	=	dataAmnt + pkgAmnt + fdAmnt;
	
	if(maxAmnt == 0)
		return Variant();
	
	ushort	actVal	=	(qrand()/(double(RAND_MAX)))*maxAmnt;
	*type	=	(actVal < dataAmnt ? 0 : (actVal < dataAmnt+pkgAmnt ? 1 : 2));
	
// 	*type	=	qMin(uchar((qrand()/(double(RAND_MAX)))*3), uchar(2));	// 0..2
// 	*type	=	qMin(uchar((qrand()/(double(RAND_MAX)))*2), uchar(1));	// 0..1 => no file descriptors yet
// 	*type	=	2;	// Only file descriptors
	Variant			ret;
	
	#define SIZE_MAX		1048576		// 1 M
// 	#define SIZE_MAX		262144		// 256 K
	
	switch(*type)
	{
		// Random data
		case 0:
		case 1:
		{
			quint32	size	=	qMax(quint32((qrand()/double(RAND_MAX))*SIZE_MAX), quint32(1));	// At least one byte
			
			QByteArray	data;
			data.reserve(size);
			
			for(qint64 j = 0; j < size; j += sizeof(int))
			{
				int	r	=	qrand();
				data.append((char*)&r, qMin(int(size - data.size()), (int)sizeof(int)));
			}
			
			ret	=	data;
		}break;
		
		// File descriptor
		case 2:
		{
			QTemporaryFile	*	file	=	new QTemporaryFile(QDir::tempPath() + QDir::separator() +  "testlocalsocket");
			
			if(!file->open())
			{
				delete file;
				qDebug("Could not create temporary file!");
				return Variant();
			}
			
			// Write filename to temp file
			QFileInfo	info(*file);
			file->write(QString("%1").arg(info.absoluteFilePath()).toUtf8());
			file->flush();
			file->seek(0);
			
			ret		=	Variant::fromSocketDescriptor(file->handle());
			*targetFile	=	file;
		}break;
	}
	
	return ret;
}


void TestLocalSocket::closeFiles(const QList< QFile * >& files)
{
	for(int i = 0; i < files.count(); i++)
		files[i]->close();
}


QTEST_MAIN(TestLocalSocket)
