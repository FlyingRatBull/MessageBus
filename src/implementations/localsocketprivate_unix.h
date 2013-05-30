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

#ifndef LOCALSOCKETPRIVATE_UNIX_H
#define LOCALSOCKETPRIVATE_UNIX_H

#include "../localsocketprivate.h"

#include <sys/types.h>
#include <sys/socket.h>

class LocalSocketPrivate_Unix : public LocalSocketPrivate
{
	public:
		LocalSocketPrivate_Unix(LocalSocket * q);
		
    virtual ~LocalSocketPrivate_Unix();
		
		/*
		 * Implementation
		 */
		// Connect to a local server
		virtual bool connectToServer(const QString& filename);
		
		// Set socket descriptor to use for communication
		virtual bool setSocketDescriptor(quintptr socketDescriptor);
		
	protected:
		/*
		 * Implementation
		 */
		// Close the connection
		virtual void close();
		
		// Currently available space in write buffer
		// Taken for sizing data for write() calls
		virtual int availableWriteBufferSpace() const;
		
		// Size of the read buffer
		// Taken for sizing data for read() calls
		virtual int readBufferSize() const;
		
		// Write data to the socket
		// size must be greater than 0
		virtual int write(const char * data, int size, quintptr * fileDescriptor);
		
		// Read data from the socket
		virtual int read(char * data, int size);
		
		// Wait for reading or writing data
		virtual bool waitForReadOrWrite(bool& readyRead, bool& readyWrite, int timeout);
		
	private:
		int							m_readBufferSize;
		fd_set					m_readFds;
		fd_set					m_writeFds;
		fd_set					m_excFds;
		struct timeval	m_timeout;
		
		struct	msghdr	m_msgHeader;
		struct	iovec		m_iovecData;
		
		char					*	m_ccmsg;
		int							m_ccmsgSize;
};

#endif // LOCALSOCKETPRIVATE_UNIX_H
