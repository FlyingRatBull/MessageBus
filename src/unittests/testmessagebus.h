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

#ifndef TESTMESSAGEBUS_H
#define TESTMESSAGEBUS_H

#include <QtTest>

#include "../MessageBus"

#include "testmessagebus_peer.h"


class TestMessageBus : public QObject
{
	Q_OBJECT
	
	public:
		TestMessageBus();
		
	public slots:
		void voidCall(MessageBus * src);
		
		void voidCall_1(MessageBus * src, const Variant& arg1);
		
		void voidCall_2(MessageBus * src, const Variant& arg1, const Variant& arg2);
		
		void voidCall_3(MessageBus * src, const Variant& arg1, const Variant& arg2, const Variant& arg3);
		
		void voidCall_4(MessageBus * src, const Variant& arg1, const Variant& arg2, const Variant& arg3, const Variant& arg4);

		void retCall(MessageBus *src, Variant *ret);
		
		void retCall_1(MessageBus * src, Variant * ret, const Variant& arg1);
		
		void retCall_2(MessageBus * src, Variant * ret, const Variant& arg1, const Variant& arg2);
		
		void retCall_3(MessageBus * src, Variant * ret, const Variant& arg1, const Variant& arg2, const Variant& arg3);
		
		void fdCall(MessageBus * src, const Variant& filename, const Variant& fileDescriptor);
	
	private slots:
		void initTestCase();
		
		void cleanupTestCase();
		
		void init();
		
		void cleanup();
		
		// Tests
		void sleep();
		
		void callVoid();
		
		void callVoid_1();
		
		void callVoid_2();
		
		void callVoid_3();
		
		void callVoid_4();
		
		void callRet();
		
		void callRet_1();
		
		void callRet_2();
		
		void callRet_3();
		
		void callBenchmark();
		
		void callHeavy();
		
		// Data
		void callVoid_data();
		
		void callVoid_1_data();
		
		void callVoid_2_data();
		
		void callVoid_3_data();
		
		void callVoid_4_data();
		
		void callRet_data();
		
		void callRet_1_data();
		
		void callRet_2_data();
		
		void callRet_3_data();
		
		void callHeavy_data();
		
	private:
		void callArgs(int num);
		
		void callArgs_random(int num);
		
		void callArgs_recall(int num, int max);
		
		void callArgs(int num, int max, bool recall);
		
	private:
		TestMessageBus_Peer			*	m_peer;
		
		MessageBus							*	m_bus;
		
		// Test data
		int								numCalls;
		
		QList<Variant>		passedArgs;
};

#endif // TESTMESSAGEBUS_H
