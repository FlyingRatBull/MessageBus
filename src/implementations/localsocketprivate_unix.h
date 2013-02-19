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


#ifndef LOCALSOCKETPRIVATE_UNIX_H
#define LOCALSOCKETPRIVATE_UNIX_H

#include "../localsocketprivate.h"

#include <sys/types.h>
#include <sys/socket.h>


class MSGBUS_LOCAL LocalSocketPrivate_Unix : public LocalSocketPrivate
{
	Q_OBJECT
	
	public:
		LocalSocketPrivate_Unix(LocalSocket *q);
		
		virtual ~LocalSocketPrivate_Unix();
		
		virtual quintptr socketDescriptor() const;
		
	protected:
		virtual bool writeSocketDescriptor(quintptr socketDescriptor);
		
		/**
		 * @brief Notification from LocalSocketPrivate that an socket descriptor is arriving on the socket.
		 *
		 * @return void
		 **/
		virtual void notifyReadSocketDescriptor();
		
		virtual quint32 writeData(const char *data, quint32 size);
		
		virtual void flush();
		
		virtual void close();
		
		virtual void open(const QString &name, QIODevice::OpenMode mode);
		
		virtual void open(quintptr socketDescriptor, bool socketOpen, QIODevice::OpenMode mode);
		
		///@todo setReadBufferSize should be implemented here
		
	private slots:
		void readData();
		
		/**
		 * @brief Actual reading of an arriving socket descriptor.
		 *
		 * @return void
		 **/
		bool readSocketDescriptor();
		
	private:
		int				m_socket;
		
		// Read notifier
		QSocketNotifier			*	m_readNotifier;
		
		// Read buffer
		QByteArray						m_readBuffer;
		
		/// Do we need to read an socket descriptor instead of normal data?
		bool									m_doReadSocketDescriptor;
		// Socket receiving
		struct	msghdr				m_recMsg;
		struct	iovec					m_recIov;
		struct	cmsghdr			*	m_recCmsg;
};

#endif // LOCALSOCKETPRIVATE_UNIX_H
