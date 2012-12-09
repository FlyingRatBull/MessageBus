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

#ifndef LOCALSOCKETPRIVATE_UNIX_WORKER_H
#define LOCALSOCKETPRIVATE_UNIX_WORKER_H

#include <QtCore/QObject>
#include <QtCore/QSocketNotifier>

#include <sys/socket.h>

#include "localsocketprivate_unix.h"

class LocalSocketPrivate_Worker	:	public QObject
{
	Q_OBJECT
	
	public:
		QSocketNotifier					m_writeNotifier;
		
		LocalSocketPrivate_Worker(LocalSocketPrivate * pp);
		
		virtual ~LocalSocketPrivate_Worker();
		
		void init();
		
	signals:
		void aborted();
		
	private slots:
		void read();
		
		bool readImpl();
		
		void exception();
		
		void write();
		
		bool writeData();
		
		bool writeSocketDescriptors();
		
	private:
		LocalSocketPrivate		*	p;
		bool										run;
		
		QSocketNotifier					m_readNotifier;
		QSocketNotifier					m_excNotifier;
		
		quint32									m_writeBytesTillNewPackage;
		
		struct	LocalSocketPrivate::Header					m_readHeader;
		QByteArray							m_tmpData;
		QByteArray							m_tmpPkg;
		
		// Socket receiving
		bool										m_reveivingSocket;
		struct	msghdr					m_recMsg;
		struct	iovec						m_recIov;
		struct	cmsghdr	*				m_recCmsg;
};

#endif // LOCALSOCKETPRIVATE_UNIX_WORKER_H
