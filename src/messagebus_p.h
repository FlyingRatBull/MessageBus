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

#ifndef MSGBUS_P_H
#define MSGBUS_P_H

#include <QRegExp>
#include <QString>

#include "localsocket.h"


enum Command
{
	CallSlot			=	0x01,
	CallSlotRet		=	0x02,
	CallRetVal		=	0x03,			// Return value from CallSlotRet
	CallRecv				=	0x04			// Call was received
};


class Variant;
class QLocalSocket;


QString socketName(const QString& service, const QString& object);

QByteArray writeVariant(const Variant& var);

Variant readVariant(const QByteArray& data, quint32& pos);


#endif // MSGBUS_P_H
