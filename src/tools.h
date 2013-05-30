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

#ifndef TOOLS_H
#define TOOLS_H

#include <QObject>
#include <QEvent>


#ifndef QT_DEBUG
#define qDebug(...)
#endif

/**
		*   Call the given \a slot of the \a object directly.
		*   Use something like Q_ARG(QString, "myString") for the arguments.
		* @param object Object to call slot from.
		* @param slot Slot to call.
		* @param arg1 Argument 1.
		* @param arg2 Argument 2.
		* @param arg3 Argument 3.
		* @param arg4 Argument 4.
		* @param arg5 Argument 5.
		*/
bool callSlotDirect ( QObject * object, const char * slot, QGenericArgument arg1 = QGenericArgument ( 0 ), QGenericArgument arg2 = QGenericArgument ( 0 ), QGenericArgument arg3 = QGenericArgument ( 0 ), QGenericArgument arg4 = QGenericArgument ( 0 ), QGenericArgument arg5 = QGenericArgument ( 0 ) );
bool callSlotDirect ( QObject * object, const char * slot, QGenericReturnArgument ret, QGenericArgument arg1 = QGenericArgument ( 0 ), QGenericArgument arg2 = QGenericArgument ( 0 ), QGenericArgument arg3 = QGenericArgument ( 0 ), QGenericArgument arg4 = QGenericArgument ( 0 ), QGenericArgument arg5 = QGenericArgument ( 0 ) );


/**
		*   Call the given \a slot of the \a object and let Qt decibe whether to use direct invokation or queued.
		*   Use something like Q_ARG(QString, "myString") for the arguments.
		* @param object Object to call slot from.
		* @param slot Slot to call.
		* @param arg1 Argument 1.
		* @param arg2 Argument 2.
		* @param arg3 Argument 3.
		* @param arg4 Argument 4.
		* @param arg5 Argument 5.
		*/
bool callSlotAuto ( QObject * object, const char * slot, QGenericArgument arg1 = QGenericArgument ( 0 ), QGenericArgument arg2 = QGenericArgument ( 0 ), QGenericArgument arg3 = QGenericArgument ( 0 ), QGenericArgument arg4 = QGenericArgument ( 0 ), QGenericArgument arg5 = QGenericArgument ( 0 ) );


/**
		*   Call the given \a slot of the \a object via Qt's signal queue.
		*   Use something like Q_ARG(QString, "myString") for the arguments.
		* @param object Object to call slot from.
		* @param slot Slot to call.
		* @param arg1 Argument 1.
		* @param arg2 Argument 2.
		* @param arg3 Argument 3.
		* @param arg4 Argument 4.
		* @param arg5 Argument 5.
		*/
bool callSlotQueued ( QObject * object, const char * slot, QGenericArgument arg1 = QGenericArgument ( 0 ), QGenericArgument arg2 = QGenericArgument ( 0 ), QGenericArgument arg3 = QGenericArgument ( 0 ), QGenericArgument arg4 = QGenericArgument ( 0 ), QGenericArgument arg5 = QGenericArgument ( 0 ) );

/**
		*   Call the given \a slot of the \a object via Qt's signal queue and blocks until the slot has been delivered.
		*   Use something like Q_ARG(QString, "myString") for the arguments.
		* @param object Object to call slot from.
		* @param slot Slot to call.
		* @param arg1 Argument 1.
		* @param arg2 Argument 2.
		* @param arg3 Argument 3.
		* @param arg4 Argument 4.
		* @param arg5 Argument 5.
		*/
bool callSlotBlockingQueued ( QObject * object, const char * slot, QGenericArgument arg1 = QGenericArgument ( 0 ), QGenericArgument arg2 = QGenericArgument ( 0 ), QGenericArgument arg3 = QGenericArgument ( 0 ), QGenericArgument arg4 = QGenericArgument ( 0 ), QGenericArgument arg5 = QGenericArgument ( 0 ) );

QString uInt64ToBase64(const quint64 number);

quint64 base64ToUInt64(const QString& string);

QString uInt64ToBase62(quint64 number);

quint64 base62ToUInt64(const QString& string);


QByteArray qEncrypt(const QByteArray& data, const QByteArray& key);

QByteArray qDecrypt(const QByteArray& data, const QByteArray& key);

#endif // HITOOLS_H
