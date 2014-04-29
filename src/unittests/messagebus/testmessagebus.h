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

#ifndef TESTMESSAGEBUS_H
#define TESTMESSAGEBUS_H

#include <QtTest>

#include "../../messagebus.h"

#include "testmessagebus_peer.h"
#include "../../tsqueue.h"


class TestMessageBus : public QObject
{
	Q_OBJECT
	
	public:
		TestMessageBus();
		
	public slots:
		void voidCall(MessageBus * src, const Variant& arg1 = Variant(), const Variant& arg2 = Variant(), const Variant& arg3 = Variant(), const Variant& arg4= Variant());
		
		void newConnection(MessageBus* bus);

	private slots:
		void init();
		
		void cleanup();
		
		// Tests
		void fixed_0();
		
		void fixed_1();
		
		void fixed_2();
		
		void fixed_3();
		
		void fixed_4();
		
		void random();
		
	private:
		void test(int min = 0, int max = 4);
		
	private:
		QList<Variant> generateArgs(int min = 4, int max = 4);
		
	private:
		QProcess								* m_peerProcess;
		
		MessageBus									*	m_interface;
		MessageBus									*	m_bus;
		QReadWriteLock						m_busLock;
		QWaitCondition						m_busReady;
		
		// Transmitted data
		TsQueue<QList<Variant> >	m_sentData;
		
		// Open temp files
		QHash<quintptr, QFile*>		m_tempFiles;
		
		QString										m_failString;
		int												m_numOpenedFiles;
};

#endif // TESTMESSAGEBUS_H
