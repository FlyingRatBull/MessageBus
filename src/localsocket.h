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

#ifndef LOCALSOCKET_H
#define LOCALSOCKET_H

#include <QObject>

#include "variant.h"

class LocalSocketPrivate;
class LocalSocket : public QObject
{
	Q_OBJECT
	
	friend class LocalSocketPrivate;
	
	public:
		LocalSocket(QObject * parent = 0);
		
		~LocalSocket();
		
		bool connectToServer(const QString& filename);
		
		bool setSocketDescriptor(quintptr socketDescriptor);
		
		quintptr socketDescriptor() const;
		
		bool isOpen() const;
		
		Variant read(bool * ok = NULL);
		
		int availableData() const;
		
		int dataToWrite() const;
		
		bool waitForReadyRead(int timeout = 30000);
		
		bool waitForDataWritten(int timeout = 30000);
		
		QString lastErrorString() const;
		
	public slots:
		void disconnectFromServer();
		
		bool write(const Variant& data);
		
		bool flush();
		
	signals:
		void connected();
		
		void disconnected();
		
		void readyRead();
		
		void dataWritten();
		
		void error(const QString& errorString);
		
	private:
		LocalSocketPrivate		*	const d_ptr;
};

#endif // LOCALSOCKET_H
