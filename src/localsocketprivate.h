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


#include <QtCore/QString>
#include <QtCore/QIODevice>
#include <QtCore/QThread>

#include "tsqueue.h"
#include "tsdataqueue.h"
#include "tools.h"

#include "localsocket.h"

#define CMD_DATA		0x10
#define CMD_PKG			0x11
#define CMD_PKG_END	0x12
#define CMD_SOCK		0x13

#ifndef LOCALSOCKETPRIVATE_H
#define LOCALSOCKETPRIVATE_H

class LocalSocketPrivate : public QThread
{
	Q_OBJECT
	
	public:
		TsDataQueue							readBuffer;
		TsDataQueue							writeBuffer;
		TsQueue<QByteArray>			readPkgBuffer;
		TsQueue<QByteArray>			writePkgBuffer;
		TsQueue<int>						readSDescBuffer;
		TsQueue<int>						writeSDescBuffer;
		
		LocalSocketPrivate(LocalSocket * q);
		
		void setSocketFilename(const QString& socketFilename);
		
		void setSocketDescriptor(int socketDescriptor);
		
		void writeData(const QByteArray& data);
		
		void writePackage(const QByteArray& package);
		
		void writeSocketDescriptor(int socketDescriptor);
		
		bool open(QIODevice::OpenMode mode);
		
		void close();
		
	signals:
		void readyRead();
		
		void readyReadPackage();
		
		void readyReadSocketDescriptor();
		
		void disconnected();
		
	protected:
		struct Header
		{
			quint32		cmd;
			quint32		size;
		};
		
		#define INIT_HEADER(_header) {_header.cmd=0;_header.size=0;}
};

#endif // LOCALSOCKETPRIVATE_H
