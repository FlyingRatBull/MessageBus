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

#ifndef LOCALSOCKETPRIVATE_H
#define LOCALSOCKETPRIVATE_H

#include "localsocket.h"

#include <QObject>
#include <QList>
#include <QElapsedTimer>
#include <QSocketNotifier>

class LocalSocketPrivate : public QObject
{
	Q_OBJECT
	
	public:
		LocalSocketPrivate(LocalSocket * q);
		
		virtual ~LocalSocketPrivate();
		
		bool waitForReadyRead(QElapsedTimer& timer, int timeout = 30000);
		
		bool waitForDataWritten(QElapsedTimer& timer, int timeout = 30000);

		void disconnectFromServer();
		
		void notifyWrite();
		
		void flush();
		
		/*
		 * Implementation
		 */
		// Connect to a local server
		virtual bool connectToServer(const QString& filename) = 0;
		
		// Set socket descriptor to use for communication
		virtual bool setSocketDescriptor(quintptr socketDescriptor) = 0;
		
		/*
		 * Control variables
		 */
		// Connected?
		bool				m_isOpen;
		// Socket descriptor
		quintptr		m_socketDescriptor;
		// Last error
		QString			m_errorString;

		/*
		 * Data variables
		 */
		// Output buffer
		QList<Variant>		m_writeBuffer;
		// Input buffer
		QList<Variant>		m_readBuffer;
		// Currently writing data (including Variant type and id)
		QByteArray				m_currentWriteData;
		
	protected:
		void enableReadNotifier();
		
		void disableReadNotifier();
		
		void removeReadNotifier();
		
		void enableWriteNotifier();
		
		void disableWriteNotifier();
		
		void removeWriteNotifier();
		
		void enableExceptionNotifier();
		
		void disableExceptionNotifier();
		
		void removeExceptionNotifier();
		
		void setOpened();
		
		void setClosed();
		
		void setError(const QString& errorString);
		
		void addReadFileDescriptor(quintptr fileDescriptor);
		
		/*
		 * Implementation
		 */
		// Close the connection
		virtual void close() = 0;
		
		// Currently available space in write buffer
		// Taken for sizing data for write() calls
		virtual int availableWriteBufferSpace() const
		{
			return 8192;
		}
		
		// Size of the read buffer
		// Taken for sizing data for read() calls
		virtual int readBufferSize() const
		{
			return 8192;
		}
		
		// Write data to the socket
		// size must be greater than 0
		virtual int write(const char * data, int size, quintptr * fileDescriptor) = 0;
		
		// Read data from the socket
		virtual int read(char * data, int size) = 0;
		
		// Wait for reading or writing data
		virtual bool waitForReadOrWrite(bool& readyRead, bool& readyWrite, int timeout) = 0;
		
	private slots:
		void readData();
		
		void writeData();
		
		void exception();
		
	private:
		// Check temporary read data and associate received file descritpors to Variants
		void checkTempReadData(bool required = false);
		
	private:
		LocalSocket				*	m_q;
		
		QSocketNotifier		*	m_readNotifier;
		QSocketNotifier		*	m_writeNotifier;
		QSocketNotifier		*	m_exceptionNotifier;
		
		/*
		 * Data variables
		 */
		// Position into currently writing data
		int								m_currentWriteDataPos;
		// File descriptor associated with the data
		quintptr				*	m_currentlyWritingFileDescriptor;
		
		// Temporary input buffer (until file descriptors are set correctly in read data)
		QList<Variant>		m_tempReadBuffer;
		// Input buffer for file descriptors which are not yet associated to Variants
		QList<quintptr>		m_tempReadFileDescBuffer;
		// Currently reading data (whole package)
		QByteArray				m_currentReadData;
		// Required size of read data for reading an package
		quint32						m_currentRequiredReadDataSize;
		// Currently reading data (one call)
		QByteArray				m_currentReadDataBuffer;
};

#endif // LOCALSOCKETPRIVATE_H
