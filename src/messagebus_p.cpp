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

#include "messagebus_p.h"

#include <QTime>

#include "localsocket.h"

#include "variant.h"

QString socketName(const QString& service, const QString& object)
{
	return QString("%1.%2").arg(service).arg(object).replace(QRegExp("[^a-zA-Z0-9_]"), "_");
}


QByteArray writeVariant(const Variant& var)
{
	QByteArray	pkg;
	int					tmp	=	0;

	// Type
	qint32	type	=	(qint32)var.type();

	pkg.append((const char*)&type, sizeof(type));

	QByteArray	data(var.toByteArray());

	// Size
	tmp	=	data.size();
	pkg.append((const char*)&tmp, sizeof(tmp));

	// Data
	pkg.append(data);

	return pkg;
}


Variant readVariant(const QByteArray &data, int &pos)
{
	if(data.size() < pos + sizeof(qint32) + sizeof(int))
	{
		qDebug("MsgBus: Data too small to read Variant!");
		pos	=	data.size();
		return Variant();
	}
	
	// Read type
	qint32	type	=	0;
	memcpy((char*)&type, data.constData() + pos, sizeof(type));
	pos	+=	sizeof(type);

	// Read size
	int	size	=	0;
	memcpy((char*)&size, data.constData() + pos, sizeof(size));
	pos	+=	sizeof(size);
	
	if(size < 1)
	{
		qDebug("MsgBus: Invalid size to read Variant!");
		pos	=	data.size();
		return Variant();
	}
	
	if(data.size() < pos + size)
	{
		qDebug("MsgBus: Data too small to read Variant!");
		pos	=	data.size();
		return Variant();
	}

	// Read data
// 	QByteArray	dat(data.mid(pos, size));
	pos	+=	size;

	return Variant(QByteArray(data.constData() + pos - size, size), (Variant::Type)type);
}
