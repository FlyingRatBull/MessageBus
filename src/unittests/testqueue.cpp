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

#include "testqueue.h"

typedef QList<QByteArray>	QByteArrayList;
Q_DECLARE_METATYPE(QByteArrayList)

#include "../tsqueue.h"
#include "../tsdataqueue.h"

#include "testqueue_enqueuerunnable.h"
#include "testqueue_dequeuerunnable.h"


void TestQueue::ts_queueItems()
{
	QFETCH(QByteArrayList, data);
	
	TsQueue<QByteArray>		queue;
	
	// List must be empty
	QVERIFY(queue.count() == 0);
	QVERIFY(queue.isEmpty());
	
	for(int i = 0; i < data.count(); i++)
	{
		queue.enqueue(data.at(i));
		
		QVERIFY(queue.last() == data.at(i));
		QVERIFY2(queue.count() == i + 1,
						 qPrintable(QString("Wrong item count! Is %1 - should be %2").arg(queue.count()).arg(i + 1)));
	}
	
	queue.clear();
	
	QVERIFY(queue.count() == 0);
	QVERIFY(queue.isEmpty());
}


void TestQueue::ts_dequeueItems()
{
	QFETCH(QByteArrayList, data);
	
	TsQueue<QByteArray>		queue;
	
	// List must be empty
	QVERIFY(queue.count() == 0);
	QVERIFY(queue.isEmpty());
	
	for(int i = 0; i < data.count(); i++)
		queue.enqueue(data.at(i));
	
	QVERIFY(queue.count() == data.count());

	while(!queue.isEmpty())
	{
		QVERIFY(queue.dequeue() == data.takeFirst());
		QVERIFY(queue.count() == data.count());
	}
	
	QVERIFY(queue.count() == 0);
	QVERIFY(queue.isEmpty());
}


void TestQueue::ts_randomAction()
{

}


void TestQueue::ts_queueData()
{
	QFETCH(QByteArrayList, data);
	
	TsDataQueue		queue;
	
	// List must be empty
	QVERIFY(queue.size() == 0);
	QVERIFY(queue.isEmpty());
	
	qint64	size	=	0;
	
	for(int i = 0; i < data.count(); i++)
	{
		queue.enqueue(data.at(i));
		
		size	+=	data.at(i).size();
		
		QVERIFY2(queue.size() == size,
						 qPrintable(QString("Wrong data size! Is %1 - should be %2").arg(queue.size()).arg(size)));
	}
	
	queue.clear();
	
	QVERIFY(queue.size() == 0);
	QVERIFY(queue.isEmpty());
}


void TestQueue::ts_dequeueData()
{
	QFETCH(QByteArrayList, data);
	
	TsDataQueue		queue;
	
	// List must be empty
	QVERIFY(queue.size() == 0);
	QVERIFY(queue.isEmpty());
	
	QByteArray	orgData;
	
	for(int i = 0; i < data.count(); i++)
	{
		queue.enqueue(data.at(i));
		orgData.append(data.at(i));
	}
	
	QVERIFY(queue.size() == orgData.size());
	
	qsrand((uint)this + QTime::currentTime().msec());

	uint	pos	=	0;
	while(!queue.isEmpty())
	{
		qint64			size	=	(qrand()/(qreal)RAND_MAX)*10000;
		QByteArray	newData(queue.dequeue(size));
		QByteArray	org(orgData.mid(pos, newData.size()));
		pos	+=	org.size();
		
// 		qDebug("nDS: %d; oS: %d; pos: %u; oDS: %d", newData.size(), org.size(), pos, orgData.size());
		
		QVERIFY(newData.size() == org.size());
		QVERIFY(newData == org);
		QVERIFY(queue.size() == orgData.size() - pos);
	}
	
	QVERIFY(queue.size() == 0);
	QVERIFY(queue.isEmpty());
}


void TestQueue::benchmark()
{
	QFETCH(QByteArrayList, data);
	
	QThreadPool	pool;
	pool.setMaxThreadCount(2);
	
	TsQueue<QByteArray>		queue;
	queue.setUseGlobal(true);
	
	// Fill queue
	for(int i = 0; i < data.count() * 64; i++)
		queue.enqueue(data.at(i/64));
	
	QRunnable	*	enqueueRunnable	=	new TestQueue_EnqueueRunnable<TsQueue<QByteArray>, QByteArray>(&queue, data);
	QRunnable	*	dequeueRunnable	=	new TestQueue_DequeueRunnable<TsQueue<QByteArray>, QByteArray>(&queue, data.count());
	
	QBENCHMARK
	{
		pool.start(enqueueRunnable);
		pool.start(dequeueRunnable);
		
		pool.waitForDone();
	}
	
	delete enqueueRunnable;
	delete dequeueRunnable;
}


void TestQueue::qt_benchmark()
{
	QFETCH(QByteArrayList, data);
	
	QThreadPool	pool;
	pool.setMaxThreadCount(2);
	
	QQueue<QByteArray>		queue;
	
	// Fill queue
	for(int i = 0; i < data.count() * 64; i++)
		queue.enqueue(data.at(i/64));
	
	QMutex	mutex;
	QRunnable	*	enqueueRunnable	=	new TestQueue_EnqueueRunnable<QQueue<QByteArray>, QByteArray>(&queue, data, &mutex);
	QRunnable	*	dequeueRunnable	=	new TestQueue_DequeueRunnable<QQueue<QByteArray>, QByteArray>(&queue, data.count(), &mutex);
	
	QBENCHMARK
	{
		pool.start(enqueueRunnable);
		pool.start(dequeueRunnable);
		
		pool.waitForDone();
	}
	
	delete enqueueRunnable;
	delete dequeueRunnable;
}


void TestQueue::ts_benchmark()
{
	QFETCH(QByteArrayList, data);
	
	QThreadPool	pool;
	pool.setMaxThreadCount(2);
	
	TsQueue<QByteArray>		queue;
	queue.setUseGlobal(false);
	
	// Fill queue
	for(int i = 0; i < data.count() * 64; i++)
		queue.enqueue(data.at(i/64));
	
	QRunnable	*	enqueueRunnable	=	new TestQueue_EnqueueRunnable<TsQueue<QByteArray>, QByteArray>(&queue, data);
	QRunnable	*	dequeueRunnable	=	new TestQueue_DequeueRunnable<TsQueue<QByteArray>, QByteArray>(&queue, data.count());
	
	QBENCHMARK
	{
		pool.start(enqueueRunnable);
		pool.start(dequeueRunnable);
		
		pool.waitForDone();
	}
	
	delete enqueueRunnable;
	delete dequeueRunnable;
}


void TestQueue::ts_queueItems_data()
{
	QTest::addColumn<QByteArrayList>("data");
	
	qsrand((uint)this);
	QByteArrayList	ret;
	
	for(int i = 0; i < 10000; i++)
	{
		qint64	size	=	(qrand()/(qreal)RAND_MAX)*10000;
		QString	name	=	QString("%1 bytes").arg(size);
		
		QByteArray	data;
		data.reserve(size);
		
		for(qint64 j = 0; j < size; j += sizeof(int))
		{
			int	r	=	qrand();
			data.append((char*)&r, qMin(size - data.size(), (qint64)sizeof(int)));
		}
		
		ret.append(data);
	}
	
	QTest::newRow("random")	<<	ret;
}


void TestQueue::ts_dequeueItems_data()
{
	ts_queueItems_data();
}


void TestQueue::ts_randomAction_data()
{
	ts_queueItems_data();
}


void TestQueue::ts_queueData_data()
{
	ts_queueItems_data();
}


void TestQueue::ts_dequeueData_data()
{
	ts_queueItems_data();
}


void TestQueue::benchmark_data()
{
	QTest::addColumn<QByteArrayList>("data");
	
	qsrand((uint)this);
	QByteArrayList	ret;
	
	for(int i = 0; i < 10000; i++)
	{
		qint64	size	=	(qrand()/(qreal)RAND_MAX)*100;
		QString	name	=	QString("%1 bytes").arg(size);
		
		QByteArray	data;
		data.reserve(size);
		
		for(qint64 j = 0; j < size; j += sizeof(int))
		{
			int	r	=	qrand();
			data.append((char*)&r, qMin(size - data.size(), (qint64)sizeof(int)));
		}
		
		ret.append(data);
	}
	
	QTest::newRow("random")	<<	ret;
}


void TestQueue::qt_benchmark_data()
{
	benchmark_data();
}


void TestQueue::ts_benchmark_data()
{
	benchmark_data();
}


QTEST_MAIN(TestQueue)
