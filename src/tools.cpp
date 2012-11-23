// Copyright 2012 Oliver Becker <der.ole.becker@gmail.com>
// 
// This file is part of the MessageBus project,
// 
// MessageBus is free software: you can redistribute it and/or modify it
// under the terms of the GNU General Public License as published by the Free Software Foundation,
// either version 3 of the License, or (at your option) any later version.
// MessageBus is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
// without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
// See the GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License along with MessageBus.
// If not, see http://www.gnu.org/licenses/.

#include "tools.h"

#include <QCoreApplication>
#include <QHash>
#include <QTimer>
#include <QMetaMethod>
#include <QStringList>


const char * HTTP_NUMBERS	=	"0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
const QString HTTP_NUMBERS_QS ( HTTP_NUMBERS );


bool callSlot ( QObject * object, const char * slot, QGenericArgument arg1, QGenericArgument arg2, QGenericArgument arg3, QGenericArgument arg4, QGenericArgument arg5, Qt::ConnectionType type )
{
	char * method = (char*)slot;
	
	while(method[0] >= '0' && method[0] <= '9')
	{
		// Only numbers: No valid slot name
		if(method[0] == '\0')
			return false;
		
		method++;
	}
	
	QByteArray	data(QString::fromAscii(method).section("(", 0, 0).trimmed().toAscii());
	method	=	data.data();

	if ( !QMetaObject::invokeMethod ( object, method, type, arg1, arg2, arg3, arg4, arg5 ) )
	{
		QMetaMethod	met ( object->metaObject()->method(object->metaObject()->indexOfSlot ( QMetaObject::normalizedSignature(slot + 1) ) ));

		QStringList	list;

		for(int i = 0; i < object->metaObject()->methodCount(); i++)
			list.append(QString(object->metaObject()->method(i).signature()));

		list.append(QString("type = \"%1\"").arg(object->metaObject()->className()));

		list.append ( "slot = \"" + QString::fromAscii ( slot + 1) + "\"" );

		if ( object->metaObject()->indexOfMethod ( QMetaObject::normalizedSignature(slot + 1) ) >= 0 )
		{
			list.append ( "method ok" );

			list.append ( "signature = \"" + QString::fromAscii ( met.signature() ) + "\"" );

			if ( object->metaObject()->indexOfSlot ( QMetaObject::normalizedSignature(slot + 1) ) >= 0 )
			{
				list.append ( "slot ok" );
			}
			else
				list.append ( "slot wrong" );
		}
		else
			list.append ( "method wrong" );

		qWarning ( "Error invoking slot \"%s\": %s", method, qPrintable ( list.join ( ", " ) ) );

		return false;
	}

	return true;
}


bool callSlotDirect ( QObject * object, const char * slot, QGenericArgument arg1, QGenericArgument arg2, QGenericArgument arg3, QGenericArgument arg4, QGenericArgument arg5 )
{
	return callSlot ( object, slot, arg1, arg2, arg3, arg4, arg5, Qt::DirectConnection );
}


bool callSlotAuto ( QObject * object, const char * slot, QGenericArgument arg1, QGenericArgument arg2, QGenericArgument arg3, QGenericArgument arg4, QGenericArgument arg5 )
{
	return callSlot ( object, slot, arg1, arg2, arg3, arg4, arg5, Qt::AutoConnection );
}


bool callSlotQueued ( QObject * object, const char * slot, QGenericArgument arg1, QGenericArgument arg2, QGenericArgument arg3, QGenericArgument arg4, QGenericArgument arg5 )
{
	return callSlot ( object, slot, arg1, arg2, arg3, arg4, arg5, Qt::QueuedConnection );
}


bool callSlotBlockingQueued ( QObject * object, const char * slot, QGenericArgument arg1, QGenericArgument arg2, QGenericArgument arg3, QGenericArgument arg4, QGenericArgument arg5 )
{
	return callSlot ( object, slot, arg1, arg2, arg3, arg4, arg5, Qt::BlockingQueuedConnection );
}


QString uInt64ToBase64 ( const quint64 number )
{
	qDebug ( "HiTools::uInt64ToBase64( number = %llu ) => \"%s\"", number, qPrintable ( QString::fromAscii ( QByteArray ( ( const char* ) &number, sizeof ( quint64 ) ).toBase64() ) ) );
	
	return QString::fromAscii ( QByteArray ( ( char* ) &number, sizeof ( quint64 ) ).toBase64() );
}


quint64 base64ToUInt64 ( const QString& string )
{
	return * ( ( quint64* ) QByteArray::fromBase64 ( string.toAscii() ).data() );
}


QString uInt64ToBase62 ( quint64 num )
{
	QString	ret;
	
	do
	{
		ret.prepend ( HTTP_NUMBERS[num%62] );
		num	/=	62;
	}
	while ( num > 0 );
	
	return ret;
}


quint64 base62ToUInt64 ( const QString& string )
{
	quint64	ret	=	0;
	int			pos	=	0;
	
	for ( int i = 0; i < string.length(); i++ )
	{
		pos	=	HTTP_NUMBERS_QS.indexOf ( string.at ( i ) );
		
		if ( pos < 0 )
			return ret;
		
		ret	*=	62;
		ret	+=	pos;
	}
	
	return ret;
}


QByteArray qEncrypt(const QByteArray& data, const QByteArray& key)
{
	QByteArray	ret;

	int	idx	=0;
	for(int i = 0; i < data.size(); i++)
		ret.append(data.at(i) ^ key.at((idx++)%key.size()));

	return ret;
}

QByteArray qDecrypt(const QByteArray& data, const QByteArray& key)
{
	return qEncrypt(data, key);
}
