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

#ifndef MESSAGEBUS_H
#define MESSAGEBUS_H

#include <QObject>
#include <QList>
#include <QReadWriteLock>
#include <QWaitCondition>

#include "variant.h"
#include "localsocket.h"
#include "localserver.h"

class MessageBus : public QObject
{
	Q_OBJECT
	
	public:
		MessageBus(QObject * callReceiver);
		
		~MessageBus();
		
		bool connectToServer(const QString& filename);
		
		void disconnectFromServer();
		
		bool listen(const QString& filename);
		
		bool isOpen() const;
		
	public slots:
		void deleteLater();
		
		bool call(const QString& slot, const QList<Variant>& paramList);
		
		bool call(const QString& slot, const Variant& param1 = Variant(), const Variant& param2 = Variant(), const Variant& param3 = Variant(), const Variant& param4 = Variant(), const Variant& param5 = Variant());
		
	signals:
		void clientConnected(MessageBus * bus);
		
		void disconnected();
		
	private slots:
		void onNewClient(quintptr socketDescriptor);
		
		void onDisconnected();
		
		void onNewPackage();
		
	private:
		bool writeHelper(const Variant& package);
		
		bool checkAckPackage();
		
		void handlePackage(Variant package);

	private:
		QObject							*	m_callReceiver;
		
		LocalServer					*	m_server;
		LocalSocket					*	m_peerSocket;
		
		// Received values
		QByteArray								m_receivingCallSlot;
		QList<Variant>						m_receivingCallArgs;
		// Received file descriptors
// 		QList<quintptr /* uid */>						m_awaitingCallFileDescriptors;
		QList<quintptr /* file descriptor */>	m_receivingCallFileDescriptors;
// 		quintptr														m_nextFileDescriptorUid;
// 		QReadWriteLock											m_fileDescriptorsLock;
// 		QWaitCondition											m_receivingCallFileDescriptorsChanged;
		
		QList<Variant>				m_tmpReadBuffer;
};

#endif // MESSAGEBUS_H
