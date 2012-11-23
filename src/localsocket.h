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

#ifndef LOCALSOCKET_H
#define LOCALSOCKET_H

#include <QtCore>
#include <QtNetwork>

class LocalSocketPrivate;

class LocalSocket : public QIODevice
{
	Q_OBJECT
	
	public:
		LocalSocket(const QString& identifier, QObject * parent = 0);
		
		LocalSocket(int socketDescriptor, QObject * parent = 0);
		
		virtual ~LocalSocket();
		
		virtual bool open(QIODevice::OpenMode mode);
		
		virtual void close();
		
		qint64 bytesAvailable() const;
		
		qint64 packagesAvailable() const;
		
		qint64 socketDescriptorsAvailable() const;
		
		QByteArray readPackage();
		
		int readSocketDescriptor();
		
		bool writePackage(const QByteArray& package);
		
		bool writeSocketDescriptor(int socketDescriptor);
		
	signals:
		void readyReadPackage();
		
		void readyReadSocketDescriptor();
		
		void disconnected();
	
	protected:
		virtual qint64 readData(char *data, qint64 maxlen);
		
		virtual qint64 writeData(const char *data, qint64 len);

	protected:
		LocalSocketPrivate		*	const	d_ptr;
		
	private:
		Q_DECLARE_PRIVATE(LocalSocket);
};

#endif // LOCALSOCKET_H
