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

#include "localsocket.h"

#include "localsocketprivate.h"


LocalSocket::LocalSocket(const QString& identifier, QObject * parent)
	:	QIODevice(parent), d_ptr(new LocalSocketPrivate(this))
{
	// Create filename
	QDir		tmpDir(QDir::temp());
	QString	filename(tmpDir.absoluteFilePath("LocalSocket_" + QString::fromAscii(QCryptographicHash::hash(identifier.toUtf8(), QCryptographicHash::Sha1).toHex()) + ".sock"));

	connect(d_ptr, SIGNAL(disconnected()), SIGNAL(disconnected()), Qt::DirectConnection);
	connect(d_ptr, SIGNAL(readyRead()), SIGNAL(readyRead()), Qt::DirectConnection);
	connect(d_ptr, SIGNAL(readyReadPackage()), SIGNAL(readyReadPackage()), Qt::DirectConnection);
	connect(d_ptr, SIGNAL(readyReadSocketDescriptor()), SIGNAL(readyReadSocketDescriptor()), Qt::DirectConnection);
	
	Q_D(LocalSocket);
	d->setSocketFilename(filename);
}


LocalSocket::LocalSocket(int socketDescriptor, QObject * parent)
	:	QIODevice(parent), d_ptr(new LocalSocketPrivate(this))
{
	connect(d_ptr, SIGNAL(disconnected()), SIGNAL(disconnected()), Qt::DirectConnection);
	connect(d_ptr, SIGNAL(readyRead()), SIGNAL(readyRead()), Qt::DirectConnection);
	connect(d_ptr, SIGNAL(readyReadPackage()), SIGNAL(readyReadPackage()), Qt::DirectConnection);
	connect(d_ptr, SIGNAL(readyReadSocketDescriptor()), SIGNAL(readyReadSocketDescriptor()), Qt::DirectConnection);
	
	Q_D(LocalSocket);
	d->setSocketDescriptor(socketDescriptor);
}


LocalSocket::~LocalSocket()
{
	if(isOpen())
		close();
	
	delete d_ptr;
}


bool LocalSocket::open(QIODevice::OpenMode mode)
{
	Q_D(LocalSocket);
	
	return d->open(mode) && QIODevice::open(mode);
}


void LocalSocket::close()
{
	Q_D(LocalSocket);
	
	d->close();
	QIODevice::close();
}


qint64 LocalSocket::bytesAvailable() const
{
	const Q_D(LocalSocket);
	
	return d->readBuffer.size() + QIODevice::size();
}


qint64 LocalSocket::packagesAvailable() const
{
	const Q_D(LocalSocket);
	
	return d->readPkgBuffer.count();
}


qint64 LocalSocket::socketDescriptorsAvailable() const
{
	const Q_D(LocalSocket);
	
	return d->readSDescBuffer.count();
}


QByteArray LocalSocket::readPackage()
{
	Q_D(LocalSocket);
	
	if(d->readPkgBuffer.isEmpty())
		return QByteArray();
	
	return d->readPkgBuffer.dequeue();
}


int LocalSocket::readSocketDescriptor()
{
	Q_D(LocalSocket);
	
	if(d->readSDescBuffer.isEmpty())
		return -1;
	
	return d->readSDescBuffer.dequeue();
}



bool LocalSocket::writePackage(const QByteArray& package)
{
	if(!isOpen() || package.size() < 1)
		return false;
	
	Q_D(LocalSocket);
	
	d->writePackage(package);
	
	return true;
}


bool LocalSocket::writeSocketDescriptor(int socketDescriptor)
{
	if(!isOpen() || socketDescriptor < 1)
		return false;
	
	Q_D(LocalSocket);
	
	d->writeSocketDescriptor(socketDescriptor);
	
	return true;
}


qint64 LocalSocket::writeData(const char *data, qint64 len)
{
	if(!isOpen())
		return -1;
	
	if(len < 1)
		return 0;
	
	Q_D(LocalSocket);

	d->writeData(QByteArray(data, len));
	
	return len;
}


qint64 LocalSocket::readData(char *data, qint64 maxlen)
{
	Q_D(LocalSocket);
	
	if(d->readBuffer.isEmpty())
		return 0;
	
	///@todo Implement TsDataQueue::dequeue(char*,uint) and use directly
	QByteArray	ret(d->readBuffer.dequeue(maxlen));
	
	memcpy(data, ret.constData(), ret.size());
	
	return ret.size();
}
