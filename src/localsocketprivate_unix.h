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

#ifndef LOCALSOCKETPRIVATE_H
#define LOCALSOCKETPRIVATE_H

#include <QtCore/QThread>
#include <QtCore/QString>
#include <QtCore/QIODevice>
#include <QtCore/QByteArray>

#include "localsocket.h"
#include "localsocketprivate.h"

#include "tsqueue.h"
#include "tsdataqueue.h"


class LocalSocketPrivate_Worker;

class LocalSocketPrivate : public QThread
{
	Q_OBJECT
	
	friend class LocalSocketPrivate_Worker;
	
	struct Header
	{
		quint32		cmd;
		quint32		size;
	};
#define INIT_HEADER(_header) {_header.cmd=0;_header.size=0;}
	
	public:
		TsDataQueue							readBuffer;
		TsDataQueue							writeBuffer;
		TsQueue<QByteArray>			readPkgBuffer;
		TsQueue<QByteArray>			writePkgBuffer;
		TsQueue<int>						readSDescBuffer;
		TsQueue<int>						writeSDescBuffer;
		
		LocalSocketPrivate(LocalSocket * q);
		
		virtual ~LocalSocketPrivate();
		
		void setSocketFilename(const QString& socketFilename);
		
		void setSocketDescriptor(int socketDescriptor);
		
// 		void notifyWrite();
// 		
// 		void notifyWritePackage();
// 		
// 		void notifyWriteSocketDescriptor();
		
		bool open(QIODevice::OpenMode mode);
		
		void close();
		
		void writeData(const QByteArray& package);
		
		void writePackage(const QByteArray& package);
		
		void writeSocketDescriptor(int socketDescriptor);
		
		void exception();
		
	signals:
		void readyRead();
		
		void readyReadPackage();
		
		void readyReadSocketDescriptor();
		
		void disconnected();
		
	protected:
		virtual void run();
		
	private slots:
		void doClose();
		
		void notifyWrite();
		
	private:
		LocalSocket		*	const	q_ptr;
		Q_DECLARE_PUBLIC(LocalSocket);
		
		LocalSocketPrivate_Worker	*	w;
		
		int							m_socket;
		int							m_maxSendPackageSize;
		int							m_socketReadBufferSize;
		QString					m_socketFilename;
};

#endif // LOCALSOCKETPRIVATE_H
