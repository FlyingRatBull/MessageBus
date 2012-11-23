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

#ifndef LOCALSERVER_H
#define LOCALSERVER_H

#include <QLocalServer>

class LocalSocket;
class LocalServerPrivate;

class LocalServer : public QLocalServer
{
	Q_OBJECT

	public:
		LocalServer(QObject * parent = 0);
		
		virtual ~LocalServer();

		bool listen(const QString& identifier);
		
	signals:
		void newConnection(quintptr socketDescriptor);
		
	protected:
		virtual void	incomingConnection (quintptr socketDescriptor);
		
	private:
		QString			m_id;
		QString			m_filename;
};

#endif // LOCALSERVER_H
