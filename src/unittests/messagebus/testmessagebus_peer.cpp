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

#include "testmessagebus_peer.h"

TestMessageBus_Peer::TestMessageBus_Peer(QObject *parent)
:	QObject(parent), interface(new MessageBusInterface("MessageBusTest", "/i")), recall(false), numCalls(0), m_org(0)
{
	interface->setReceiver(this);
	
	connect(interface, SIGNAL(newConnection(MessageBus*)), SLOT(setOrg(MessageBus*)));
}


TestMessageBus_Peer::~TestMessageBus_Peer()
{
	interface->deleteLater();
}


bool TestMessageBus_Peer::waitForNumCalls(int num, int stepTimeout)
{
	QReadLocker		locker(&numCallsLock);
	
	if(numCalls >= num)
		return true;
	
	QElapsedTimer	timer;
	timer.start();
	while(numCalls < num)
	{
		QCoreApplication::processEvents();
		// Return if we did not receive data or if we received an failure
		timer.restart();
		
		quint64	old	=	numCalls;
		if(!numCallsChanged.wait(&numCallsLock, stepTimeout))
		{
			if(old == numCalls)
			{
				if(timer.elapsed() >= stepTimeout)
					break;
				else
					QCoreApplication::processEvents();
			}
		}
	}
	
	return (numCalls >= num);
}


void TestMessageBus_Peer::voidCall(MessageBus *src)
{
	raiseNumCalls();
	
	if(recall)
		src->call("voidCall");
}


void TestMessageBus_Peer::voidCall_1(MessageBus *src, const Variant &arg1)
{
	raiseNumCalls();

	passedArgs.append(arg1);
	
	if(recall)
		src->call("voidCall_1", arg1);
}


void TestMessageBus_Peer::voidCall_2(MessageBus *src, const Variant &arg1, const Variant &arg2)
{
	raiseNumCalls();

	passedArgs.append(arg1);
	passedArgs.append(arg2);
	
	if(recall)
		src->call("voidCall_2", arg1, arg2);
}


void TestMessageBus_Peer::voidCall_3(MessageBus *src, const Variant &arg1, const Variant &arg2, const Variant &arg3)
{
	raiseNumCalls();

	passedArgs.append(arg1);
	passedArgs.append(arg2);
	passedArgs.append(arg3);
	
	if(recall)
		src->call("voidCall_3", arg1, arg2, arg3);
}


void TestMessageBus_Peer::voidCall_4(MessageBus *src, const Variant &arg1, const Variant &arg2, const Variant &arg3, const Variant &arg4)
{
	raiseNumCalls();

	passedArgs.append(arg1);
	passedArgs.append(arg2);
	passedArgs.append(arg3);
	passedArgs.append(arg4);
	
	if(recall)
		src->call("voidCall_4", arg1, arg2, arg3, arg4);
}


void TestMessageBus_Peer::retCall(MessageBus *src, Variant *ret)
{
// 	qDebug("retCall");
	
	raiseNumCalls();
	
	(*ret)	=	true;
	
	if(recall)
	{
		m_recallFunction	=	"retCall";
		QTimer::singleShot(0, this, SLOT(doRecall()));
	}
}


void TestMessageBus_Peer::retCall_1(MessageBus *src, Variant *ret, const Variant &arg1)
{
	raiseNumCalls();
	
	(*ret)	=	arg1;

	passedArgs.append(arg1);
	
	if(recall)
	{
		m_recallFunction	=	"retCall_1";
		QTimer::singleShot(0, this, SLOT(doRecall()));
	}
}


void TestMessageBus_Peer::retCall_2(MessageBus *src, Variant *ret, const Variant &arg1, const Variant &arg2)
{
	raiseNumCalls();
	
	(*ret)	=	arg1;

	passedArgs.append(arg1);
	passedArgs.append(arg2);
	
	if(recall)
	{
		m_recallFunction	=	"retCall_2";
		QTimer::singleShot(0, this, SLOT(doRecall()));
	}
}


void TestMessageBus_Peer::retCall_3(MessageBus *src, Variant *ret, const Variant &arg1, const Variant &arg2, const Variant &arg3)
{
	raiseNumCalls();
	
	(*ret)	=	arg1;

	passedArgs.append(arg1);
	passedArgs.append(arg2);
	passedArgs.append(arg3);
	
	if(recall)
	{
		m_recallFunction	=	"retCall_3";
		QTimer::singleShot(0, this, SLOT(doRecall()));
	}
}


void TestMessageBus_Peer::doRecall()
{
	if(passedArgs.count() == 0)
		retVals.append(m_org->callRet(m_recallFunction));
	if(passedArgs.count() == 1)
		retVals.append(m_org->callRet(m_recallFunction, passedArgs.at(0)));
	if(passedArgs.count() == 2)
		retVals.append(m_org->callRet(m_recallFunction, passedArgs.at(0), passedArgs.at(1)));
	if(passedArgs.count() == 3)
		retVals.append(m_org->callRet(m_recallFunction, passedArgs.at(0), passedArgs.at(1), passedArgs.at(2)));
}


void TestMessageBus_Peer::setOrg(MessageBus *org)
{
	m_org	=	org;
}


void TestMessageBus_Peer::raiseNumCalls()
{
	numCallsLock.lockForWrite();
	numCalls++;
	numCallsChanged.wakeAll();
	numCallsLock.unlock();
}
