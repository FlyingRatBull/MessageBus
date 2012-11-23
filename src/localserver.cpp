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

#include "localserver.h"

#include <QCryptographicHash>
#include <QDir>
#include <QFile>

#include "localsocket.h"

LocalServer::LocalServer(QObject * parent)
		:	QLocalServer(parent)
{
}


LocalServer::~LocalServer()
{
	QLocalServer::close();
	
	QFile::remove(m_filename);
	
	m_id.clear();
	m_filename.clear();
}


bool LocalServer::listen(const QString& identifier)
{
	QDir		tmpDir(QDir::temp());
	QString	filename(tmpDir.absoluteFilePath("LocalSocket_" + QString::fromAscii(QCryptographicHash::hash(identifier.toUtf8(), QCryptographicHash::Sha1).toHex()) + ".sock"));

	QLocalServer::removeServer(filename);
	bool	ret	=	QLocalServer::listen(filename);
	
// 	dbg("LocalServer::listen");
	
	if(ret)
	{
		QFile::setPermissions(filename, QFile::ExeOwner | QFile::ExeGroup | QFile::ExeOther |
																	QFile::ReadOwner | QFile::ReadGroup | QFile::ReadOther |
																	QFile::WriteOwner | QFile::WriteGroup | QFile::WriteOther);
		
		m_id				=	identifier;
		m_filename	=	filename;
	}
	else
		qWarning("Could not create local server socket!");
	
	return ret;
}


void LocalServer::incomingConnection(quintptr socketDescriptor)
{
	emit(newConnection(socketDescriptor));
}
