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

#ifndef MESSAGEBUSINTERFACE_H
#define MESSAGEBUSINTERFACE_H

#include <QObject>

#include "messagebus.h"


class MessageBusInterfacePrivate;

class MessageBusInterface : public QObject
{
	Q_OBJECT
	
	friend class MessageBusInterfacePrivate;

	public:
		MessageBusInterface(const QString service, const QString& object, QObject * obj);

		virtual ~MessageBusInterface();

		bool isValid() const;
		
	signals:
		void newConnection(MessageBus * msgBus);

	private:
		MessageBusInterfacePrivate			*	d;
};

#endif // MESSAGEBUSINTERFACE_H
