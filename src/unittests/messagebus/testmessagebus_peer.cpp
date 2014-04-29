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

#include "testmessagebus_peer.h"

#include <QCoreApplication>
#include <QDir>

int main(int argc, char ** argv)
{
	QCoreApplication		app(argc, argv);
	TestMessageBus_Peer		peer;
	
	return app.exec();
}


TestMessageBus_Peer::TestMessageBus_Peer()
{
	m_bus	=	new MessageBus(this);
	
	if(!m_bus->connectToServer(QDir::tempPath() +  "/test_callbus.sock"))
		qFatal("Could not connect to interface!");
	
// 	qDebug("Peer: Connected: %s", (m_bus->isOpen() ? "true" : "false"));
	
	connect(m_bus, SIGNAL(disconnected()), SLOT(onDisconnected()));
}


TestMessageBus_Peer::~TestMessageBus_Peer()
{

}


void TestMessageBus_Peer::voidCall(MessageBus *src, const Variant &arg1, const Variant &arg2, const Variant &arg3, const Variant &arg4)
{
	int	argsCount	=	0;
	
	if(arg1.isValid())
	{
		argsCount++;
		
		if(arg2.isValid())
		{
			argsCount++;
			
			if(arg3.isValid())
			{
				argsCount++;
				
				if(arg4.isValid())
					argsCount++;
			}
		}
	}
	
// 	qDebug("Peer: voidCall() (args: %d; arg1: %s)", argsCount, qPrintable(arg1.toString()));

	if(!src->call("voidCall", arg1, arg2, arg3, arg4))
		qFatal("Peer: call() failed!");
	
// 	qDebug("Peer: done");
}


void TestMessageBus_Peer::onDisconnected()
{
	qDebug("TestMessageBus_Peer::onDisconnected()");
	
	QCoreApplication::exit();
}
