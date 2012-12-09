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

#ifndef MESSAGEBUSINTERFACE_P_H
#define MESSAGEBUSINTERFACE_P_H

#include <QtCore>

#include "localserver.h"
#include "localsocket.h"

// #include <sys/socket.h>
// #include <sys/un.h>

#include "variant.h"
#include "messagebusinterface.h"
#include "messagebus.h"
#include "messagebus_p.h"
#include "tools.h"


class MessageBusInterfacePrivate : public LocalServer
{
	Q_OBJECT

	public:
		MessageBusInterfacePrivate(QObject * obj, MessageBusInterface * parentObject)
				:	LocalServer(obj),
				object(obj), p(parentObject)
		{
		}


		~MessageBusInterfacePrivate()
		{
		}


	public:
		virtual bool listen(const QString& name)
		{
			removeServer(name);

			if(LocalServer::listen(name))
			{
				s_sockets.append(name);
				return true;
			}

			return false;
		}


	protected:
		virtual void incomingConnection(quintptr socketDescriptor)
		{
// 				dbg("MsgBusInterfacePrivate::incomingConnection()");
			LocalSocket	*	socket	=	new LocalSocket(socketDescriptor, this);

			MessageBus	*	bus	=	new MessageBus(object, socket, this);

			emit(p->newConnection(bus));
		}


	public:
		MessageBusInterface		*	p;
		QObject						*	object;

	public:
		static QStringList				s_sockets;
};

#endif // MESSAGEBUSINTERFACE_P_H
