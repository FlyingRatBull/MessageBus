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

#include "messagebusinterface.h"

#include "messagebusinterface_p.h"


MessageBusInterface::MessageBusInterface(const QString service, const QString& object, QObject * obj)
		:	QObject(obj), d(new MessageBusInterfacePrivate(obj, this))
{
	if(!d->listen(socketName(service, object)))
		qWarning("Cannot listen to '%s': %s", qPrintable(socketName(service, object)), qPrintable(d->errorString()));
}


MessageBusInterface::~MessageBusInterface()
{
	delete d;
}


bool MessageBusInterface::isValid() const
{
	return d->isListening();
}


void MessageBusInterface::setReceiver(QObject *obj)
{
	d->setReceiver(obj);
}


QStringList			MessageBusInterfacePrivate::s_sockets;


int __cleanupMessageBusInterfacePrivate()
{
	foreach(const QString& name, MessageBusInterfacePrivate::s_sockets)
		QLocalServer::removeServer(name);

	return 1;
}


Q_DESTRUCTOR_FUNCTION(__cleanupMessageBusInterfacePrivate);
