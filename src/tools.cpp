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

#include "tools.h"

#include <QCoreApplication>
#include <QTimer>
#include <QMetaMethod>
#include <QStringList>

#include <malloc.h>

#include "global.h"

#include <qvarlengtharray.h>


MSGBUS_LOCAL	const char *	HTTP_NUMBERS	=	"0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
const QString	HTTP_NUMBERS_QS ( HTTP_NUMBERS );

bool invokeMethod(QObject *obj,
                           const char *member,
                           Qt::ConnectionType type,
                           QGenericReturnArgument ret,
                           QGenericArgument val0 = QGenericArgument(),
                           QGenericArgument val1 = QGenericArgument(),
                           QGenericArgument val2 = QGenericArgument(),
                           QGenericArgument val3 = QGenericArgument(),
                           QGenericArgument val4 = QGenericArgument(),
                           QGenericArgument val5 = QGenericArgument(),
                           QGenericArgument val6 = QGenericArgument(),
                           QGenericArgument val7 = QGenericArgument(),
                           QGenericArgument val8 = QGenericArgument(),
                           QGenericArgument val9 = QGenericArgument())
	
{
	
if (!obj)
	
    return false;
	
	
QVarLengthArray<char, 512> sig;
	
int len = qstrlen(member);
	
if (len <= 0)
	
    return false;
	
sig.append(member, len);
	
sig.append('(');
	
	
const char *typeNames[] = {ret.name(), val0.name(), val1.name(), val2.name(), val3.name(),
	
                           val4.name(), val5.name(), val6.name(), val7.name(), val8.name(),
	
                           val9.name()};
	
	
int paramCount;
	
for (paramCount = 1; paramCount < 10; ++paramCount) {
	
    len = qstrlen(typeNames[paramCount]);
	
    if (len <= 0)
	
        break;
	
    sig.append(typeNames[paramCount], len);
	
    sig.append(',');
	
}
	
if (paramCount == 1)
	
    sig.append(')'); // no parameters
	
else
	
    sig[sig.size() - 1] = ')';
	
sig.append('\0');
	
	
int idx = obj->metaObject()->indexOfMethod(sig.constData());
	
if (idx < 0) {
	
    QByteArray norm = QMetaObject::normalizedSignature(sig.constData());
	
    idx = obj->metaObject()->indexOfMethod(norm.constData());
	
}
	
	
if (idx < 0 || idx >= obj->metaObject()->methodCount()) {
	
    qWarning("QMetaObject::invokeMethod: No such method %s::%s",
	
             obj->metaObject()->className(), sig.constData());
	
    return false;
	
}
	
QMetaMethod method = obj->metaObject()->method(idx);
	
return method.invoke(obj, type, ret,
	
                     val0, val1, val2, val3, val4, val5, val6, val7, val8, val9);
	
}


bool callSlot ( QObject * object, const char * slot, QGenericReturnArgument ret, QGenericArgument arg1, QGenericArgument arg2, QGenericArgument arg3, QGenericArgument arg4, QGenericArgument arg5, Qt::ConnectionType type )
{
	const	int	slotLength	=	strlen(slot);
	char		*	method			=	(char*)malloc(slotLength + 1);
	int				length			=	0;
	
	while(slot[length] >= '0' && slot[length] <= '9')
		length++;
	
	// Copy from real slot name begin until end (incl. EOF)
	memcpy(method, slot + length, slotLength - length + 1);
	length	=	0;
	
	// Only numbers: No valid slot name
	if(method[0] == '\0')
	{
		free(method);
		return false;
	}
	
	while(method[length] != '\0' && method[length] != '(' && method[length] != ' ')
		length++;
	
	// We got the end of the slot name => put EOF
	method[length]	=	'\0';

// 	if(!QMetaObject::invokeMethod(object, method, type, ret, arg1, arg2, arg3, arg4, arg5))
	if(!invokeMethod(object, method, type, ret, arg1, arg2, arg3, arg4, arg5))
	{
		QMetaMethod	met ( object->metaObject()->method(object->metaObject()->indexOfSlot ( QMetaObject::normalizedSignature(slot + 1) ) ));

		QStringList	list;

		for(int i = 0; i < object->metaObject()->methodCount(); i++)
			list.append(QString(object->metaObject()->method(i).methodSignature()));

		list.append(QStringLiteral("type = \"%1\"").arg(object->metaObject()->className()));

		list.append ( "slot = \"" + QString::fromLatin1 ( slot + 1) + "\"" );

		if ( object->metaObject()->indexOfMethod ( QMetaObject::normalizedSignature(slot + 1) ) >= 0 )
		{
			list.append ( "method ok" );

			list.append ( "signature = \"" + QString::fromLatin1 ( met.methodSignature() ) + "\"" );

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

		free(method);
		return false;
	}

	free(method);
	return true;
}


bool callSlotDirect ( QObject * object, const char * slot, QGenericArgument arg1, QGenericArgument arg2, QGenericArgument arg3, QGenericArgument arg4, QGenericArgument arg5 )
{
	return callSlot ( object, slot, QGenericReturnArgument(), arg1, arg2, arg3, arg4, arg5, Qt::DirectConnection );
}


bool callSlotDirect ( QObject * object, const char * slot, QGenericReturnArgument ret, QGenericArgument arg1, QGenericArgument arg2, QGenericArgument arg3, QGenericArgument arg4, QGenericArgument arg5 )
{
	return callSlot ( object, slot, ret, arg1, arg2, arg3, arg4, arg5, Qt::DirectConnection );
}


bool callSlotAuto ( QObject * object, const char * slot, QGenericArgument arg1, QGenericArgument arg2, QGenericArgument arg3, QGenericArgument arg4, QGenericArgument arg5 )
{
	return callSlot ( object, slot, QGenericReturnArgument(), arg1, arg2, arg3, arg4, arg5, Qt::AutoConnection );
}


bool callSlotQueued ( QObject * object, const char * slot, QGenericArgument arg1, QGenericArgument arg2, QGenericArgument arg3, QGenericArgument arg4, QGenericArgument arg5 )
{
	return callSlot ( object, slot, QGenericReturnArgument(), arg1, arg2, arg3, arg4, arg5, Qt::QueuedConnection );
}


bool callSlotBlockingQueued ( QObject * object, const char * slot, QGenericArgument arg1, QGenericArgument arg2, QGenericArgument arg3, QGenericArgument arg4, QGenericArgument arg5 )
{
	return callSlot ( object, slot, QGenericReturnArgument(), arg1, arg2, arg3, arg4, arg5, Qt::BlockingQueuedConnection );
}


QString uInt64ToBase64 ( const quint64 number )
{
	return QString::fromLatin1 ( QByteArray ( ( char* ) &number, sizeof ( quint64 ) ).toBase64() );
}


quint64 base64ToUInt64 ( const QString& string )
{
	return * ( ( quint64* ) QByteArray::fromBase64 ( string.toLatin1() ).data() );
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
