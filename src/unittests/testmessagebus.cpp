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

#include "testmessagebus.h"

TestMessageBus::TestMessageBus(): QObject()
{
	m_peer			=	0;
	m_bus				=	0;
}


void TestMessageBus::voidCall(MessageBus *src)
{
	numCalls++;
}


void TestMessageBus::voidCall_1(MessageBus *src, const Variant &arg1)
{
	numCalls++;

	passedArgs.append(arg1);
}


void TestMessageBus::voidCall_2(MessageBus *src, const Variant &arg1, const Variant &arg2)
{
	numCalls++;

	passedArgs.append(arg1);
	passedArgs.append(arg2);
}


void TestMessageBus::voidCall_3(MessageBus *src, const Variant &arg1, const Variant &arg2, const Variant &arg3)
{
	numCalls++;

	passedArgs.append(arg1);
	passedArgs.append(arg2);
	passedArgs.append(arg3);
}


void TestMessageBus::voidCall_4(MessageBus *src, const Variant &arg1, const Variant &arg2, const Variant &arg3, const Variant &arg4)
{
	numCalls++;

	passedArgs.append(arg1);
	passedArgs.append(arg2);
	passedArgs.append(arg3);
	passedArgs.append(arg4);
}


void TestMessageBus::retCall(MessageBus *src, Variant *ret)
{
	numCalls++;
	
	(*ret)	=	true;
}


void TestMessageBus::retCall_1(MessageBus *src, Variant *ret, const Variant &arg1)
{
	numCalls++;
	
	(*ret)	=	arg1;

	passedArgs.append(arg1);
}


void TestMessageBus::retCall_2(MessageBus *src, Variant *ret, const Variant &arg1, const Variant &arg2)
{
	numCalls++;
	
	(*ret)	=	arg1;

	passedArgs.append(arg1);
	passedArgs.append(arg2);
}


void TestMessageBus::retCall_3(MessageBus *src, Variant *ret, const Variant &arg1, const Variant &arg2, const Variant &arg3)
{
	numCalls++;
	
	(*ret)	=	arg1;

	passedArgs.append(arg1);
	passedArgs.append(arg2);
	passedArgs.append(arg3);
}


void TestMessageBus::fdCall(MessageBus *src, const Variant &filename, const Variant &fileDescriptor)
{
	numCalls++;
	
	passedArgs.append(filename);
	passedArgs.append(fileDescriptor);
}


void TestMessageBus::initTestCase()
{
	// Init peer
	m_peer	=	new TestMessageBus_Peer();
	
	// Wait for peer to be running
	while(!m_peer->isRunning() || m_peer->interface == 0)
		QTest::qWait(100);
	
	QVERIFY(m_peer->interface->isValid());
	
	m_bus	=	new MessageBus("MessageBusTest", "/i", this);
	QVERIFY2(m_bus->isOpen(), "Cannot connect to MessageBus-Interface");
	
	QCoreApplication::processEvents();
}


void TestMessageBus::cleanupTestCase()
{
	if(m_peer)
		m_peer->deleteLater();
	
	if(m_bus)
		m_bus->deleteLater();
}


void TestMessageBus::init()
{
	m_peer->recall		=	false;
	m_peer->numCalls	=	0;
	m_peer->retVals.clear();
	m_peer->passedArgs.clear();
	
	numCalls	=	0;
	passedArgs.clear();
}


void TestMessageBus::cleanup()
{
}


void TestMessageBus::sleep()
{
// 	qDebug("(0) Waiting 15 seconds ...");
// 	::sleep(15);
}


void TestMessageBus::callVoid()
{
	QFETCH(QList<Variant>, args);
	QFETCH(bool, recall);
	
	m_peer->recall	=	recall;
	
	QString	call;
	
	if(!args.isEmpty())
		call	=	QString("voidCall_%1").arg(args.count());
	else
		call	=	QString("voidCall");
	
	m_bus->call(call, args);
	
// 	qDebug("(1) Waiting 15 seconds ...");
// 	QTest::qWait(15000);
	
	int	i	=	0;
	while(m_peer->numCalls < 1 && i++ < 500)
		QTest::qWait(10);
	
	QVERIFY2(m_peer->numCalls == 1, "Could not call function");
	
	QVERIFY2(m_peer->passedArgs.count() == args.count(), "Invalid number of arguments received");
	
	for(int i = 0; i < m_peer->passedArgs.count(); i++)
		QVERIFY2(m_peer->passedArgs[i] == args[i], "Invalid arguments received");
	
	if(recall)
	{
		int	i	=	0;
		while(numCalls < 1 && i++ < 500)
			QTest::qWait(10);
		
		QVERIFY2(numCalls == 1, "No function recall");
		
		QVERIFY2(passedArgs.count() == args.count(), "Invalid number of arguments received on recall");
		QVERIFY2(passedArgs == args, "Invalid arguments received on recall");
	}
}


void TestMessageBus::callVoid_1()
{
	callVoid();
}


void TestMessageBus::callVoid_2()
{
	callVoid();
}


void TestMessageBus::callVoid_3()
{
	callVoid();
}


void TestMessageBus::callVoid_4()
{
	callVoid();
}


void TestMessageBus::callRet()
{
	QFETCH(QList<Variant>, args);
	QFETCH(bool, recall);
	
	m_peer->recall	=	recall;
	
	QString	call;
	
	if(!args.isEmpty())
		call	=	QString("retCall_%1").arg(args.count());
	else
		call	=	QString("retCall");
	
	Variant	ret	=	m_bus->callRet(call, args);
	
	QVERIFY2(m_peer->numCalls == 1, "Could not call function");
	
	QVERIFY2(m_peer->passedArgs.count() == args.count(), "Invalid number of arguments received");
	
	for(int i = 0; i < m_peer->passedArgs.count(); i++)
		QVERIFY2(m_peer->passedArgs[i] == args[i], "Invalid arguments received");
	
	if(args.isEmpty())
	{
		QVERIFY2(ret.isValid(), "Invalid return value (1)");
		QVERIFY2(ret.toBool(), "Invalid return value (2)");
	}
	else
		QVERIFY2(ret == args.first(), "Invalid return value");
	
	if(recall)
	{
		int	i	=	0;
		while(numCalls < 1 && m_peer->retVals.isEmpty() && i++ < 500)
			QTest::qWait(10);
		
		QVERIFY2(numCalls == 1, "No function recall");
		
		QVERIFY2(passedArgs.count() == args.count(), "Invalid number of arguments received on recall");
		QVERIFY2(passedArgs == args, "Invalid arguments received on recall");
		QVERIFY2(m_peer->retVals.count() == 1, "Invalid return value on recall (1)");
		
		if(args.isEmpty())
			QVERIFY2(m_peer->retVals.first().toBool(), "Invalid return value on recall (2)");
		else
			QVERIFY2(m_peer->retVals.first() == args.first(), "Invalid return value on recall (2)");
	}
}


void TestMessageBus::callRet_1()
{
	callRet();
}


void TestMessageBus::callRet_2()
{
	callRet();
}


void TestMessageBus::callRet_3()
{
	callRet();
}


void TestMessageBus::callBenchmark()
{
	int	calls	=	0;
	QBENCHMARK
	{
		m_bus->call("voidCall");
		calls++;
	}
	
	int	i	=	0;
	while(m_peer->numCalls < calls && i++ < 50)
		QTest::qWait(100);
}


void TestMessageBus::callHeavy()
{
	QFETCH(QList<QList<Variant> >, args);
	int	calls	=	0;
	
	for(int i = 0; i < 1000; i++)
	{
		for(int j = 0; j < args.count(); j++)
		{
			m_bus->call("fdCall", args[j][0], args[j][1]);
			calls++;
		}
	}
	
	int	i	=	0;
	while(m_peer->numCalls < calls && i++ < 50)
		QTest::qWait(100);
	
	QVERIFY2(m_peer->passedArgs.count() == args.count(), "Invalid number of arguments received");
	
	for(int i = 0; i < m_peer->passedArgs.count(); i++)
		QVERIFY2(m_peer->passedArgs[i] == args[i], "Invalid arguments received");
	
	// Delete temp files
	for(int j = 0; j < args.count(); j++)
	{
		QString	filename;
		QFile		fdFile(QString("/proc/self/fd/%1").arg(args[j][1]));
		filename	=	fdFile.readAll();
		
		close(args[1]);
		QFile::remove(filename);
	}
}


void TestMessageBus::callVoid_data()
{
	callArgs(0);
}


void TestMessageBus::callVoid_1_data()
{
	callArgs(1);
}


void TestMessageBus::callVoid_2_data()
{
	callArgs(2);
}


void TestMessageBus::callVoid_3_data()
{
	callArgs(3);
}


void TestMessageBus::callVoid_4_data()
{
	callArgs(4);
}


void TestMessageBus::callRet_data()
{
	callArgs(0);
}


void TestMessageBus::callRet_1_data()
{
	callArgs(1);
}


void TestMessageBus::callRet_2_data()
{
	callArgs(2);
}


void TestMessageBus::callRet_3_data()
{
	callArgs(3);
}


void TestMessageBus::callArgs(int num)
{
	callArgs_random(num);
}


void TestMessageBus::callHeavy_data()
{
	QTest::addColumn<QList<QList<Variant> > >("args");
	QList<QList<Variant> >	args;
	
	for(int i = 0; i < 10; i++)
	{
		QList<Variant>	params;

		QTemporaryFile	file;
		file.setAutoRemove(false);
		params.append(file.fileName());		// Path
		int	fd	=	fopen(file.fileName(), "r");
		params.append(Variant::fromSocketDescriptor(fd));
		
		args.append(params);
	}
	
	QTest::newRow("heavy fd") << args;
}


void TestMessageBus::callArgs_random(int num)
{
	callArgs_recall(num, 0);
	
	// 5 calls with random number of arguments
	callArgs_recall(-1, 3);
	callArgs_recall(-1, 3);
	callArgs_recall(-1, 3);
	callArgs_recall(-1, 3);
	callArgs_recall(-1, 3);
}


void TestMessageBus::callArgs_recall(int num, int max)
{
	callArgs(num, max, false);
	callArgs(num, max, true);
}



void TestMessageBus::callArgs(int num, int max, bool recall)
{
	QTest::addColumn<QList<Variant> >("args");
	QTest::addColumn<bool>("recall");
	QList<Variant>	args;
	
	// Random number of arguments
	if(num < 0)
		num	=	qRound((qrand()/(qreal)RAND_MAX)*max);
	
	QString	callN(", num = %1%2");
	QString	callName("%1" + callN.arg(num).arg(recall ? ", recall" : ""));
#define DATA_NAME(a) qPrintable(callName.arg(a))
	
	// Strings
	args.clear();
	for(int i = 0; i < num; i++)
		args.append(QString("Teststring"));
	QTest::newRow(DATA_NAME("strings")) << args << recall;
	
	// Int8
	args.clear();
	for(int i = 0; i < num; i++)
		args.append((qint8)qrand());
	QTest::newRow(DATA_NAME("int8")) << args << recall;
	
	// Int16
	args.clear();
	for(int i = 0; i < num; i++)
		args.append((qint16)qrand());
	QTest::newRow(DATA_NAME("int16")) << args << recall;
	
	// Int32
	args.clear();
	for(int i = 0; i < num; i++)
		args.append((qint32)qrand());
	QTest::newRow(DATA_NAME("int32")) << args << recall;
	
	// Int64
	args.clear();
	for(int i = 0; i < num; i++)
		args.append((qint64)qrand());
	QTest::newRow(DATA_NAME("int64")) << args << recall;
	
	// UInt8
	args.clear();
	for(int i = 0; i < num; i++)
		args.append((quint8)qrand());
	QTest::newRow(DATA_NAME("uint8")) << args << recall;
	
	// UInt16
	args.clear();
	for(int i = 0; i < num; i++)
		args.append((quint16)qrand());
	QTest::newRow(DATA_NAME("uint16")) << args << recall;
	
	// UInt32
	args.clear();
	for(int i = 0; i < num; i++)
		args.append((quint32)qrand());
	QTest::newRow(DATA_NAME("uint32")) << args << recall;
	
	// UInt64
	args.clear();
	for(int i = 0; i < num; i++)
		args.append((quint64)qrand());
	QTest::newRow(DATA_NAME("uint64")) << args << recall;
	
	// ByteArray (max 1 Mb)
	qint64	size	=	(qrand()/(qreal)RAND_MAX)*(1024*1024);
	args.clear();
	for(int i = 0; i < num; i++)
	{
		QByteArray	data;
		data.reserve(size);
		
		for(qint64 j = 0; j < size; j += sizeof(int))
		{
			int	r	=	qrand();
			data.append((char*)&r, qMin(size - data.size(), (qint64)sizeof(int)));
		}
		args.append(data);
	}
	QTest::newRow(DATA_NAME("bytearray")) << args << recall;
}

QTEST_MAIN(TestMessageBus);
