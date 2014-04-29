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

#ifndef PEEROBJECT_H
#define PEEROBJECT_H

#include <QtNetwork>

#include "../../localsocket.h"
#include "../../variant.h"


class PeerObject : public QObject
{
	Q_OBJECT
	
	public:
		PeerObject(const QString &identifier);
		
		virtual ~PeerObject();
		
	private:
		void init(const QString &identifier);
		
	private slots:
		void close(int code = 0);
		
		void closeDelayed();
		
		void readStdin();
		
		void readData();
		
		void readPackage();
		
		void readSocketDescriptor();
		
		void checkReadData();
		
	private:
		LocalSocket			*	m_socket;
		
		QLocalSocket		*	m_peerSocket;
		QReadWriteLock		m_peerStreamLock;
		
		QTextStream													*	m_cout;
		
		QReadWriteLock												m_dataDescriptionLock;
		QList<QPair<QChar, QByteArray> >			m_dataDescription;
		
		QReadWriteLock												m_dataLock;
		QList<QByteArray>											m_data;
		
		QReadWriteLock												m_dataPkgLock;
		QList<QByteArray>											m_dataPkg;
		
		QReadWriteLock												m_dataFDLock;
		QStringList														m_dataFD;
		
		bool																	m_close;
		
		// For debugging only
		QFile																	m_dbgLog;
		
	private:
		static QHash<QChar, QString>		s_actionTitles;
};

#endif // PEEROBJECT_H
