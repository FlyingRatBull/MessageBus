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

#ifndef TESTMSGBUS_PEER_H
#define TESTMSGBUS_PEER_H

#include <QObject>

#include "../../MessageBus"
#include "../../MessageBusInterface"


class TestMessageBus_Peer : public QObject
{
	Q_OBJECT
	
	public:
		explicit TestMessageBus_Peer(QObject *parent = 0);
		
		virtual ~TestMessageBus_Peer();
		
		bool waitForNumCalls(int num, int stepTimeout);
		
		MessageBusInterface		*	interface;
		
		bool							recall;
		
		int								numCalls;
		QReadWriteLock		numCallsLock;
		QWaitCondition		numCallsChanged;
		
		QList<Variant>		passedArgs;
		
		QList<Variant>		retVals;
		
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
		
	private slots:
		void doRecall();
		
		void setOrg(MessageBus * org);
		
	private:
		void raiseNumCalls();
		
	private:
		MessageBus	*	m_org;
		QString				m_recallFunction;
};

#endif // TESTMSGBUS_PEER_H
