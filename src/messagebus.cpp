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

#include "messagebus.h"

#include "messagebus_p.h"
#include "tools.h"

#include <QCoreApplication>
#include <unistd.h>

#define PKG_TYPE_CALL  0x01
#define PKG_TYPE_PARAM 0x02
#define PKG_TYPE_END   0x03
#define PKG_TYPE_END_ACKNOWLEDGED	0x04
#define PKG_TYPE_ACK   0x05


MessageBus::MessageBus(QObject* callReceiver)
:	QObject(callReceiver), m_callReceiver(callReceiver), m_server(0), m_peerSocket(0)
{

}


MessageBus::~MessageBus()
{
// 	qDebug("MessageBus::~MessageBus()");
}


bool MessageBus::connectToServer(const QString& filename)
{
	QWriteLocker		socketLocker(&m_socketLock);
	
	if(m_peerSocket)
		return false;
	
	LocalSocket	*	socket	=	new LocalSocket(this);
// 	socket->setWritePkgBufferSize(10485760 /* 10M */);
	
// 	qDebug("Connecting to: %s", qPrintable(filename));
	
	bool	result	=	socket->connectToServer(filename);
	
	if(result)
	{
		connect(socket, SIGNAL(disconnected()), SLOT(onDisconnected()), Qt::QueuedConnection);
		connect(socket, SIGNAL(readyRead()), SLOT(onNewPackage()), Qt::QueuedConnection);
		
		m_peerSocket	=	socket;
	}
	else
		socket->deleteLater();
	
	return result;
}


void MessageBus::disconnectFromServer()
{
	QWriteLocker		socketLocker(&m_socketLock);
	
	if(!m_peerSocket)
		return;
	
	m_peerSocket->disconnectFromServer();
}


bool MessageBus::listen(const QString& filename)
{
	QWriteLocker		socketLocker(&m_socketLock);
	
	if(m_peerSocket || m_server)
		return false;
	
	m_server	=	new LocalServer(this);
	
	bool	result	=	m_server->listen(filename);
	
	if(result)
	{
// 		qDebug("Listening on %s", qPrintable(filename));
		connect(m_server, SIGNAL(newConnection(quintptr)), SLOT(onNewClient(quintptr)));
	}
	else
	{
		m_server->deleteLater();
		m_server	=	0;
	}
	
	return result;
}


bool MessageBus::isOpen() const
{
	QReadLocker		socketLocker(&m_socketLock);
	
	return (m_peerSocket && m_peerSocket->isOpen());
}


void MessageBus::deleteLater()
{
	QReadLocker		socketLocker(&m_socketLock);
	
	if(m_peerSocket)
		m_peerSocket->disconnectFromServer();
	
	m_tmpReadBuffer.clear();
	m_receivingCallArgs.clear();
	
	disconnect(this, 0, 0, 0);
	
	QObject::deleteLater();
}


bool MessageBus::call(const QString& slot, const QList< Variant >& paramList)
{
	QReadLocker		socketLocker(&m_socketLock);
	
	if(!m_peerSocket)
		return false;
	
	/*
	 * Send CALL package
	 */
	Variant	package(slot);
	// Set id
	package.setOptionalId(PKG_TYPE_CALL);
	
	if(!writeHelper(package))
		return false;
// 	static	int	_cw_count	=	0;
// 	_cw_count++;
// 	qDebug("CALL package sent: %d - %s", _cw_count, qPrintable(paramList.first().toString()));
	
	/*
	 * Send parameters
	 */
	foreach(Variant parameter, paramList)
	{
		// Set id
		parameter.setOptionalId(PKG_TYPE_PARAM);
		
		if(!writeHelper(parameter))
			return false;
// 		qDebug("PARAM package sent");
	}
	
	/*
	 * Send END package
	 */
	package	=	Variant();
	// Set id
	package.setOptionalId(PKG_TYPE_END);
	
	if(!writeHelper(package))
		return false;
// 	m_peerSocket->flush();
// 	qDebug("END package sent");
	
	/*
	 * Wait for ACK package
	 */
	{
		disconnect(m_peerSocket, SIGNAL(readyRead()), this, 0);
		
		while(m_peerSocket && m_peerSocket->isOpen() && !checkAckPackage())
		{
			socketLocker.unlock();
			m_peerSocket->waitForReadyRead(1000);
			socketLocker.relock();
		}
		
		connect(m_peerSocket, SIGNAL(readyRead()), SLOT(onNewPackage()), Qt::QueuedConnection);
		callSlotQueued(this, "onNewPackage");
	}
	//qDebug("Waited for ACK package");
	
	// Ack received
	return true;
}


bool MessageBus::call(const QString& slot, const Variant& param1, const Variant& param2, const Variant& param3, const Variant& param4, const Variant& param5)
{
	QList<Variant>	params;
	
	if(param1.isValid())
	{
		params.append(param1);
		
		if(param2.isValid())
		{
			params.append(param2);
			
			if(param3.isValid())
			{
				params.append(param3);
				
				if(param4.isValid())
				{
					params.append(param4);
					
					if(param5.isValid())
						params.append(param5);
				}
			}
		}
	}
	
	return call(slot, params);
}


void MessageBus::onNewClient(quintptr socketDescriptor)
{
	LocalSocket	*	socket	=	new LocalSocket(this);
	if(!socket->setSocketDescriptor(socketDescriptor))
	{
		socket->deleteLater();
		return;
	}
	
	MessageBus	*	bus	=	new MessageBus(m_callReceiver);
	bus->m_peerSocket = socket;
// 	socket->setWritePkgBufferSize(10485760 /* 10M */);
	
	connect(socket, SIGNAL(disconnected()), bus, SLOT(onDisconnected()), Qt::QueuedConnection);
	connect(socket, SIGNAL(readyRead()), bus, SLOT(onNewPackage()), Qt::QueuedConnection);
	
	emit(clientConnected(bus));
}


void MessageBus::onDisconnected()
{
	QWriteLocker		socketLocker(&m_socketLock);
	
	if(!m_peerSocket)
		return;
	
// 	qDebug("MessageBus::onDisconnected()");
	
	LocalSocket	*	socket	=	0;
	socket	=	m_peerSocket;
	m_peerSocket = 0;
	socketLocker.unlock();
	
 	emit(disconnected());
	
	socket->deleteLater();
}


void MessageBus::onNewPackage()
{
	QReadLocker		socketLocker(&m_socketLock);
	
// 	qDebug("onNewPackage()");
	if(!m_peerSocket)
		return;
	
	// Read new packages
	while(m_peerSocket->availableData() > 0)
	{
		// Read package
		Variant		package(m_peerSocket->read());
	
		m_tmpReadBuffer.enqueue(package);
	}
	socketLocker.unlock();
	
	while(!m_tmpReadBuffer.isEmpty())
		handlePackage(m_tmpReadBuffer.dequeue());
}


bool MessageBus::writeHelper(const Variant &package)
{
	QReadLocker		socketLocker(&m_socketLock);
	//qDebug("Writing package of size %d", package.size());
	
	while(m_peerSocket && !m_peerSocket->write(package))
	{
		if(!m_peerSocket->isOpen())
			return false;
		
		m_peerSocket->waitForDataWritten(1000);
	}
	
	return (m_peerSocket != 0);
}


bool MessageBus::checkAckPackage()
{
	// socket lock should already be locked by call()
	
	if(!m_peerSocket->availableData())
		return false;
	
	Variant	package(m_peerSocket->read());
	
	// Get type
	const quint32 type = package.optionalId();
	
	if(type == PKG_TYPE_ACK)
		return true;
	else if(type == PKG_TYPE_END)
	{
		// Send ACK package
		Variant	ackPackage;
		ackPackage.setOptionalId(PKG_TYPE_ACK);
		
		if(!writeHelper(ackPackage))
			return false;
//  				m_peerSocket->flush();
// 		qDebug("ACK package sent");
		package.setOptionalId(PKG_TYPE_END_ACKNOWLEDGED);
	}
	
	m_tmpReadBuffer.enqueue(package);
	
	return false;
}


void MessageBus::handlePackage(Variant package)
{
	// Get type
	const quint32 type = package.optionalId();
	
// 	qDebug("Package size: %d", package.size());
// 	qDebug("Package type: 0x%02X", type);

	switch(type)
	{
		/*
		* CALL package
		*/
		case PKG_TYPE_CALL:
		{
			static	int	_rc_count	=	0;
			_rc_count++;
//  			qDebug("CALL package received: %d", _rc_count);
			
			// Clean previous values
			m_receivingCallSlot.clear();
// 			qDebug("CALL package received: clearing");
			m_receivingCallArgs.clear();

			m_receivingCallSlot	=	package.toString().toAscii();
// 			if(m_receivingCallSlot.isEmpty())
// 				qDebug("CALL package received: received empty call slot!");
		}break;
		
		/*
		* END package
		*/
		case PKG_TYPE_END:
		{
			// Send ACK package
			Variant	ackPackage;
			ackPackage.setOptionalId(PKG_TYPE_ACK);
			
			if(!writeHelper(ackPackage))
				return;
//  				m_peerSocket->flush();
// 			qDebug("ACK package sent");
		} // No break here!
		case PKG_TYPE_END_ACKNOWLEDGED:
		{
// 			static	int	_re_count	=	0;
// 			_re_count++;
// 			if(m_receivingCallArgs.count())
// 				qDebug("END package received: %d - %s", _re_count, qPrintable(m_receivingCallArgs.first().toString()));
// 			else
// 				qDebug("END package received: %d", _re_count);
			//qDebug("Slot: %s", m_receivingCallSlot.constData());
			
			if(m_receivingCallSlot.isEmpty())
			{
// 				qDebug("END package received: empty call slot!");
				m_receivingCallArgs.clear();
				return;
			}
			
			// Call
			if(m_receivingCallArgs.count() == 0)
				callSlotQueued(m_callReceiver, m_receivingCallSlot.constData(), Q_ARG(MessageBus*, this));
			else if(m_receivingCallArgs.count() == 1)
				callSlotQueued(m_callReceiver, m_receivingCallSlot.constData(), Q_ARG(MessageBus*, this), Q_ARG(Variant, m_receivingCallArgs.at(0)));
			else if(m_receivingCallArgs.count() == 2)
				callSlotQueued(m_callReceiver, m_receivingCallSlot.constData(), Q_ARG(MessageBus*, this), Q_ARG(Variant, m_receivingCallArgs.at(0)), Q_ARG(Variant, m_receivingCallArgs.at(1)));
			else if(m_receivingCallArgs.count() == 3)
				callSlotQueued(m_callReceiver, m_receivingCallSlot.constData(), Q_ARG(MessageBus*, this), Q_ARG(Variant, m_receivingCallArgs.at(0)), Q_ARG(Variant, m_receivingCallArgs.at(1)), Q_ARG(Variant, m_receivingCallArgs.at(2)));
			else if(m_receivingCallArgs.count() == 4)
				callSlotQueued(m_callReceiver, m_receivingCallSlot.constData(), Q_ARG(MessageBus*, this), Q_ARG(Variant, m_receivingCallArgs.at(0)), Q_ARG(Variant, m_receivingCallArgs.at(1)), Q_ARG(Variant, m_receivingCallArgs.at(2)), Q_ARG(Variant, m_receivingCallArgs.at(3)));
			else
				qWarning("MessageBus: Too many arguments!");
			
			m_receivingCallSlot.clear();
// 			qDebug("END package received: clearing");
			m_receivingCallArgs.clear();
		}break;
		
		/*
			* PARAM package
			*/
		case PKG_TYPE_PARAM:
		{
//  			qDebug("PARAM package received");
			package.setOptionalId(0);
			
			m_receivingCallArgs.append(package);
		}break;
	}
}


int __init_MessageBus()
{
	qRegisterMetaType<MessageBus*>("MessageBus*");
	return 1;
}

Q_CONSTRUCTOR_FUNCTION(__init_MessageBus);
