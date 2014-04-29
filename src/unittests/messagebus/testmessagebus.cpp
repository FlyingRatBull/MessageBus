/*
 *  MessageBus - Inter process communication library
 *  Copyright (C) 2013  Oliver Becker <der.ole.becker@gmail.com>
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

#include "testmessagebus.h"

#include <unistd.h>

#define RUNTIME_SECONDS 20

TestMessageBus::TestMessageBus()
	:	QObject(), m_bus(0), m_interface(0), m_peerProcess(0)
{

}


void TestMessageBus::voidCall(MessageBus *src, const Variant &arg1, const Variant &arg2, const Variant &arg3, const Variant &arg4)
{
// 	qDebug("TestMessageBus::voidCall()");
	
	// Check if we already got an error
	if(!m_failString.isEmpty())
		return;
	
	QList<Variant>		testData(m_sentData.dequeue());
	QList<Variant>		args(QList<Variant>() << arg1 << arg2 << arg3 << arg4);
	
	// Check if parameters match
	for(int i = 0; i < testData.count(); i++)
	{
		if(args.count() <= i)
		{
			m_failString	=	"Too few arguments in voidCall()!";
			qDebug("TestMessageBus::voidCall() - Too few arguments!");
			return;
		}
		
// 		qDebug("  arg%d: %s", (i+1), qPrintable(args[i].toString()));
		
		if(testData[i].type() != args[i].type())
		{
			m_failString	=	"Invalid type in voidCall()!";
      qDebug("TestMessageBus::voidCall() - invalid type: 0x%02X != 0x%02X", testData[i].type(), args[i].type());
			return;
		}
		
		if(testData[i].type() == Variant::SocketDescriptor)
		{
			bool			ret	=	true;
			QFile	*		testFile	=	m_tempFiles[testData[i].toSocketDescriptor()];
			QFileInfo	info(*testFile);
			QFile			argFile;
			
			// Try to open file
			if(args[i].toSocketDescriptor() > 0 && argFile.open(args[i].toSocketDescriptor(), QIODevice::ReadOnly))
			{
				// Get filename
				QString	filename(QString::fromUtf8(argFile.readAll().trimmed()));
				
				// Compare filenames
				ret	=	(!filename.isEmpty() && filename == info.absoluteFilePath());
				
				argFile.close();
				::close(args[i].toSocketDescriptor());
			}
			else
				ret	=	false;
			
			// Delete file
			if(testFile)
			{
				testFile->close();
				::close(testFile->handle());
				delete testFile;
			}
			m_tempFiles.remove(testData[i].toSocketDescriptor());
			
			if(!ret)
			{
				m_failString	=	"Invalid file data in voidCall()!";
				qDebug("TestMessageBus::voidCall() - invalid file data!");
				return;
			}
		}
		else
		{
			if(testData[i] != args[i])
			{
				m_failString	=	"Invalid data in voidCall()!";
				qDebug("TestMessageBus::voidCall() - invalid data (types: 0x%02X/0x%02X; data:\n  %s (%d)\n  %s (%d))!", args[i].type(), testData[i].type(), qPrintable(args[i].toString()), args[i].size(), qPrintable(testData[i].toString()), testData[i].size());
				return;
			}
		}
	}
}


void TestMessageBus::newConnection(MessageBus* bus)
{
	QVERIFY2(m_bus == 0, "Too many connecting clients!");
	
	QWriteLocker	locker(&m_busLock);
	m_bus	=	bus;
	m_busReady.wakeAll();
}


void TestMessageBus::init()
{
	m_interface	=	new MessageBus(this);
	QVERIFY2(m_interface->listen(QDir::tempPath() +  "/test_callbus.sock"), "Cannot create MessageBus-Interface!");
	
	connect(m_interface, SIGNAL(clientConnected(MessageBus*)), SLOT(newConnection(MessageBus*)));
	
	m_peerProcess	=	new QProcess();
	m_peerProcess->setProcessChannelMode(QProcess::ForwardedChannels);
	m_peerProcess->start(QCoreApplication::applicationFilePath().replace("test_messagebus", "test_messagebus_peer"));
	
	QVERIFY2(m_peerProcess->waitForStarted(), "Could not wait for peer process to be started!");
	
	// Wait for new client
	QReadLocker		locker(&m_busLock);
	QElapsedTimer	timer;
	timer.start();
	while(timer.elapsed() < /*10000*/ 100000 && !m_bus)
	{
		if(m_busReady.wait(&m_busLock, 100))
			break;

		// Process events so thaht MessageBusInterface::newConnection() gets fired
		locker.unlock();
		QCoreApplication::processEvents();
		locker.relock();
	}
	
	QVERIFY2(m_bus != 0, "Failed to wait for new client!");
	
	m_numOpenedFiles	=	0;
	
	QCoreApplication::processEvents();
}


void TestMessageBus::cleanup()
{
	m_peerProcess->terminate();
	QVERIFY2(m_peerProcess->waitForFinished(), "Could not wait for peer process to be finished!");
	m_peerProcess->close();
	m_peerProcess->deleteLater();
	m_peerProcess	=	0;
	
	// Delete MessageBus
// 	qDebug("m_bus->deleteLater();");
	m_bus->deleteLater();
	m_bus	=	0;
	
	// Delete interface
	m_interface->deleteLater();
	m_interface	=	0;
	
	// Delete temporary files
	foreach(QFile * file, m_tempFiles.values())
		delete file;
	m_tempFiles.clear();
	
	QCoreApplication::processEvents();
}


void TestMessageBus::fixed_0()
{
	test(0, 0);
}


void TestMessageBus::fixed_1()
{
	test(1, 1);
}


void TestMessageBus::fixed_2()
{
	test(2, 2);
}


void TestMessageBus::fixed_3()
{
	test(3, 3);
}


void TestMessageBus::fixed_4()
{
	test(4, 4);
}


void TestMessageBus::random()
{
	test(0, 4);
}


void TestMessageBus::test(int min, int max)
{
	QElapsedTimer		timer;
	timer.start();
	
	quint64			count	=	0;
	qint64			elapsed	=	0;
	qint64			reduction	=	0;
	
	while(timer.elapsed() < RUNTIME_SECONDS*1000)
	{
		QVERIFY2(m_bus->isOpen(), "MessageBus is closed!");
		
		// Exclude generation time
		qint64	tmpTime	=	timer.elapsed();
		QList<Variant>		args(generateArgs(min, max));
		reduction	+=	timer.elapsed()-tmpTime;
		
		m_sentData.enqueue(args);
		
// 		qDebug("Calling: (%d args)",args.count());
		
		QVERIFY2(m_bus->call("voidCall", args), "call() failed!");
		count++;
		
		QVERIFY2(m_failString.isEmpty(), qPrintable(m_failString));
		
		QCoreApplication::processEvents();
		
		// Let only be 1000 calls in the queue
		while(m_sentData.count() > 1000 && timer.elapsed() < RUNTIME_SECONDS*1000)
		{
			QCoreApplication::processEvents(QEventLoop::AllEvents, 100);
			sleep(0);
		}
	}
// 	qDebug("Waiting");
	
	// Wait for remaining calls
	while(!m_sentData.isEmpty())
	{
		int	oldCount	=	m_sentData.count();
		QVERIFY2(m_failString.isEmpty(), qPrintable(m_failString));
		
		QElapsedTimer	t;
		t.start();
		while(t.elapsed() < 5000 && oldCount == m_sentData.count())
		{
			QCoreApplication::processEvents(QEventLoop::AllEvents, 500);
			sleep(0);
		}
		
		QVERIFY2(m_sentData.count() < oldCount, qPrintable(QString("Failed to wait for new calls (%1/%2 missing)!").arg(m_sentData.count()).arg(count)));
	}
	
	QVERIFY2(m_tempFiles.isEmpty(), "Still temporary files available!");
	
	elapsed	=	timer.elapsed() - reduction;
	
	qDebug("Sent %llu calls in ~%lld seconds (~%u CpS; ~%02.04f ms/C)", count, quint64(elapsed/1000.0), uint(count/(elapsed/1000.0)), float(elapsed)/count);
}


QList< Variant > TestMessageBus::generateArgs(int min, int max)
{
#define MAX_NUM_TYPES 4
// Maximum data size
#define SIZE_MAX		102400		// 100k
	
	QList<Variant>	ret;
	// Number of arguments
	uchar	numArgs	=	((float(qrand())/float(RAND_MAX))*(max-min))+min;
	
	for(int i = 0; i < numArgs; i++)
	{
		// Number of different types
		uchar		type	=	(float(qrand())/float(RAND_MAX))*MAX_NUM_TYPES;
		Variant	tmp;
		
		switch(type)
		{
			// String
			case 0:
			{
				// Random hash string
				int	value	=	qrand();
				tmp	=	QString::fromAscii(QCryptographicHash::hash(QByteArray((const char*)&value, sizeof(value)), QCryptographicHash::Md5).toHex());
				
// 				qDebug("Data: %s", qPrintable(tmp.toString()));
			}break;
			
			// Integer
			case 1:
			{
				tmp	=	qrand();
			}break;
			
			// Data
			case 2:
			{
				quint32	size	=	qMax(quint32((qrand()/double(RAND_MAX))*SIZE_MAX), quint32(1));	// At least one byte
				
				QByteArray	data;
				data.reserve(size);
				
				for(qint64 j = 0; j < size; j += sizeof(int))
				{
					int	r	=	qrand();
					data.append((char*)&r, qMin(int(size - data.size()), (int)sizeof(int)));
				}
				
				tmp	=	data;
			}break;
			
			// File descriptor
			case 3:
			{
				QTemporaryFile	*	file	=	new QTemporaryFile(QDir::tempPath() + QDir::separator() +  "testlocalsocket");
				
				// Stop at 100 files to not reach user limit
				if(m_numOpenedFiles++ > 100 || !file->open())
				{
					delete file;
					tmp	=	"This would be a file";
// 					qDebug("Could not create temporary file!");
					break;
				}
				
				// Write filename to temp file
				QFileInfo	info(*file);
				file->write(QString("%1").arg(info.absoluteFilePath()).toUtf8());
				file->flush();
				file->seek(0);
				
				tmp		=	Variant::fromSocketDescriptor(file->handle());
				m_tempFiles[file->handle()]	=	file;
			}break;
		}
		
		ret.append(tmp);
	}
	
	return ret;
}

QTEST_MAIN(TestMessageBus);
