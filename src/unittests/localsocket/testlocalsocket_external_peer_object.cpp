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

#include "testlocalsocket_external_peer_object.h"

#include <stdio.h>
#include <sys/signal.h>

#include "../../tools.h"
#include "../logger.h"

QHash<QChar, QString>		PeerObject::s_actionTitles;

PeerObject::PeerObject(const QString &identifier)
	:	QObject(), m_close(false), m_dbgLog("/var/tmp/localsocket_peerobject.log")
{
	m_dbgLog.open(QIODevice::WriteOnly);

	// Set log file
	QFile::remove("/tmp/testlocalsocket_peer.log");
	Logger::setLogFile("/tmp/testlocalsocket_peer.log");

	init(identifier);
}


PeerObject::~PeerObject()
{
// 	qDebug("=== ~PeerObject() ===");

	if(m_socket)
		m_socket->close();

	if(m_peerSocket) {
		m_peerSocket->close();

		if(m_peerSocket->state() != QLocalSocket::UnconnectedState)
			m_peerSocket->waitForDisconnected();

		delete m_peerSocket;
		m_peerSocket	=	0;
	}
}


void PeerObject::init(const QString &identifier)
{
	// m_dbgLog.write("PeerObject::init()\n");
	// m_dbgLog.flush();

	// QLocalSocket throws SIGPIE
	signal(SIGPIPE, SIG_IGN);
	// Ignore closed terminal
	signal(SIGHUP, SIG_IGN);

	// Fill titles
	s_actionTitles['d']	=	"Data";
	s_actionTitles['p']	=	"Package";
	s_actionTitles['s']	=	"Socket descriptor";

	m_peerSocket	=	new QLocalSocket();
	connect(m_peerSocket, SIGNAL(readyRead()), SLOT(readStdin()), Qt::DirectConnection);
	connect(m_peerSocket, SIGNAL(disconnected()), SLOT(closeDelayed()));

	m_peerSocket->connectToServer("TestLocalSocket_Peer");

	if(!m_peerSocket->waitForConnected()) {
		qFatal("Peer: Cannot connect to QLocalServer!");
	}

// 	qDebug("Peer: Control socket connected!");

	m_socket	=	new LocalSocket(this);
	connect(m_socket, SIGNAL(readyRead()), SLOT(readData()));
	connect(m_socket, SIGNAL(readyReadPackage()), SLOT(readPackage()));
	connect(m_socket, SIGNAL(readyReadSocketDescriptor()), SLOT(readSocketDescriptor()));
	connect(m_socket, SIGNAL(disconnected()), SLOT(closeDelayed()));

	m_socket->connectToServer(identifier, QIODevice::ReadWrite);
// 	qDebug("Peer: connectToServer fired");

	if(!m_socket->waitForConnected()) {
		// m_dbgLog.write("Failed to open LocalSocket for reading!\n");
		// m_dbgLog.flush();
		qFatal("Peer: Failed to open LocalSocket for reading!");
	}

// 	qDebug("Peer: LocalSocket connected!");
}


void PeerObject::close(int code)
{
// 	qDebug("=== close() ===");

	disconnect(m_socket, 0, this, 0);
	disconnect(m_peerSocket, 0, this, 0);

	m_socket->close();
	m_peerSocket->close();

	if(m_peerSocket->state() != QLocalSocket::UnconnectedState)
		m_peerSocket->waitForDisconnected();

	QCoreApplication::exit(code);
}


void PeerObject::closeDelayed()
{
	QTimer::singleShot(5000, this, SLOT(close()));
}


void PeerObject::readStdin()
{
	QReadLocker		readLocker(&m_peerStreamLock);

	// m_dbgLog.write("PeerObject::readStdin()\n");
	// m_dbgLog.flush();

// 	qDebug("PeerObject::readStdin()");

	while(m_peerSocket->canReadLine()) {
		QString				line(m_peerSocket->readLine());

		QChar					cmd(line.at(0));
		QByteArray		data(QByteArray::fromHex(line.mid(1).toAscii()));

		// Close?
		if(cmd == 'c') {
// 			qDebug("===== Received close!");
			disconnect(m_peerSocket, SIGNAL(readyRead()), this, SLOT(readStdin()));

// 			m_dbgLog.write("PeerObject::readStdin() - close\n");
// 			m_dbgLog.flush();
			m_close	=	true;

			// m_dbgLog.write("Locking: 97\n");// m_dbgLog.flush();
			QReadLocker	readLocker(&m_dataDescriptionLock);
			// m_dbgLog.write("Ok: 97\n");// m_dbgLog.flush();

			if(m_dataDescription.isEmpty()) {
// 				m_dbgLog.write("c: close!\n");
// 				m_dbgLog.flush();
				close();
			}

			return;
		}

		static int recCount	=	0;
		recCount++;
		Logger::log("Read commands", recCount);
// 		m_dbgLog.write("Received: ");
// 		m_dbgLog.write(QString("(%1) %2").arg(++recCount).arg(cmd).toAscii());
// 		m_dbgLog.write(data);
// 		m_dbgLog.write("\n");
// 		m_dbgLog.flush();

		// m_dbgLog.write("Locking: 113\n");// m_dbgLog.flush();
		QWriteLocker		writeLock(&m_dataDescriptionLock);
		// m_dbgLog.write("Ok: 113\n");// m_dbgLog.flush();
		m_dataDescription.append(QPair<QChar, QByteArray>(cmd, data));
		writeLock.unlock();

		callSlotQueued(this, "checkReadData");
	}
}


void PeerObject::readData()
{
// 	static int readCount	=	0;
// 	m_dbgLog.write("PeerObject::readData()");
// 	m_dbgLog.write(QString("(%1)\n").arg(++readCount).toAscii());
// 	m_dbgLog.flush();

	static quint64	totalRead	=	0;

	while(m_socket->bytesAvailable()) {
		// Read size
		quint32	orgSize	=	0;
		quint32	size		=	0;

		if(m_socket->bytesAvailable() < sizeof(size)) {
			callSlotQueued(this, "checkReadData");
			return;
		}

		if(m_socket->read((char *)&orgSize, sizeof(orgSize)) < sizeof(orgSize))
			qFatal("Could not read size!");

		size	=	qFromBigEndian<quint32>(orgSize);
// 		if(m_socket->read((char*)&size, sizeof(size)) < sizeof(size))
// 			qFatal("Could not checksum!");
// 		size	=	qFromBigEndian<quint32>(size);

// 		qDebug("Peer: size to read: %u", size);

		if(size == 0) {
			qDebug("Peer: PeerObject::readData() - zero size data");

			m_peerSocket->write(QString("fReceived zero size").toAscii());
// 			m_peerSocket->flush();

			if(m_close)
				close(1);

			return;
		}

		// Read data with specified size
		QElapsedTimer	timer;
		QByteArray		data(size, (char)'\0');

// 		qDebug("PeerObject::readData() - size: %u", size);

		// m_dbgLog.write("Reading: ");
		// m_dbgLog.write(QString("%1 bytes").arg(size).toAscii());
		// m_dbgLog.write("\n");
		// m_dbgLog.flush();

		// Read max 30 seconds
		timer.start();
		qint64	tmp		=	0;
		qint64	read	=	0;
		bool		waiting	=	false;

		while(read < size && timer.elapsed() < 10000) {
			if(m_socket->bytesAvailable() < 1) {
				waiting	=	true;
// 				Logger::log("Read bytes (TestLocalSocket_Ext_PO)", totalRead + read, "bW", "Begin waiting for bytes");
				if(m_socket->waitForReadyRead(10000 - timer.elapsed()))
					timer.restart();
			}

			tmp	=	m_socket->read(data.data() + read, size - read);

			if(tmp < 0) {
				m_peerSocket->write(QString("fCould not read data!").toAscii());
// 				m_peerSocket->flush();

				close(1);
				return;
			}

// 			Logger::log("Read tmp bytes (TestLocalSocket_Ext_PO)", tmp);

			read	+=	tmp;
		}

// 		if(waiting)
// 			Logger::log("Read bytes (TestLocalSocket_Ext_PO)", totalRead + read, "eW", "End waiting for bytes");

		// m_dbgLog.write("Ok\n");
		// m_dbgLog.flush();

		// Check read data size
		if(read < size) {
			qDebug("Peer: PeerObject::readData() - not enough data");

			m_peerSocket->write(QString("fDid not receive enough data. Expected: %1; Got: %2").arg(size).arg(data.size()).toAscii());
// 			m_peerSocket->flush();

// 			m_dbgLog.write("Did not receive enough data");
// 			m_dbgLog.write("\n");
// 			m_dbgLog.flush();

			if(m_close)
				close(1);

			return;
		}

// 		printf("=== Data read (%lld/%d) ===\n", read, data.size());
// 		QByteArray	tmpData(QByteArray((const char*)&orgSize, sizeof(orgSize)) + data);
// 		for(int i = 0; i < tmpData.size(); i++)
// 			printf("%02X", tmpData.constData()[i]);
// 		printf("\n=================\n");

		// m_dbgLog.write("Locking: 170\n");// m_dbgLog.flush();
		QWriteLocker		writeLocker(&m_dataLock);
		// m_dbgLog.write("Ok: 170\n");// m_dbgLog.flush();

		// Store data
		m_data.append(data);

		static int totalReadCount	=	0;
		totalReadCount++;
		Logger::log("Handled data", totalReadCount);
	}

	callSlotQueued(this, "checkReadData");
}


void PeerObject::readPackage()
{
	// 	static int readCount	=	0;
	// 	m_dbgLog.write("PeerObject::readData()");
	// 	m_dbgLog.write(QString("(%1)\n").arg(++readCount).toAscii());
	// 	m_dbgLog.flush();

	static quint64	totalRead	=	0;

	QByteArray	package(m_socket->readPackage());

	if(package.isEmpty()) {
		qDebug("Peer: PeerObject::readPackage() - zero size data");

		m_peerSocket->write(QString("fReceived zero size").toAscii());
// 		m_peerSocket->flush();

		if(m_close)
			close(1);

		return;
	}

	QWriteLocker		writeLocker(&m_dataPkgLock);
	m_dataPkg.append(package);

	static int totalReadPkgCount	=	0;
	totalReadPkgCount++;
	Logger::log("Handled packages", totalReadPkgCount);

	callSlotQueued(this, "checkReadData");
}


void PeerObject::readSocketDescriptor()
{
	int	fd	=	m_socket->readSocketDescriptor();

// 	qDebug(">>> readSocketDescriptor() : %x", fd);

	QFile		file;

	// Try to open file
	if(!file.open(fd, QIODevice::ReadOnly))
	{
		file.seek(0);
		qDebug("Peer: PeerObject::readSocketDescriptor() - coudl not open file");

		m_peerSocket->write(QString("fReceived invalid file descriptor").toAscii());
// 		m_peerSocket->flush();

		if(m_close)
			close(1);

		return;
	}

	// Get filename
	QString	filename(QString::fromUtf8(file.readAll().trimmed()));

// 	qDebug("File is: %s (size: %lld)", qPrintable(filename), file.size());

	// Save filename
	QWriteLocker		writeLocker(&m_dataFDLock);
	m_dataFD.append(filename);

	// Close file
	file.close();

	static int totalReadFDCount	=	0;
	totalReadFDCount++;
	Logger::log("Handled file descriptors", totalReadFDCount);

	callSlotQueued(this, "checkReadData");
}


void PeerObject::checkReadData()
{
// 	qDebug("[%p] PeerObject::checkReadData() - data: %d; stdin: %d", QCoreApplication::instance(), m_data.size(), m_dataDescription.size());

// 	m_dbgLog.write("PeerObject::checkReadData()\n");
// 	m_dbgLog.flush();

	// m_dbgLog.write("Locking: 200\n");// m_dbgLog.flush();
	QWriteLocker		readDescriptionLock(&m_dataDescriptionLock);

	while(!m_dataDescription.isEmpty() && (!m_data.isEmpty() || !m_dataPkg.isEmpty() || !m_dataFD.isEmpty())) {
		QPair<QChar, QByteArray>	description(m_dataDescription.takeFirst());
		QByteArray		data;

		if(description.first == 'd') {
			QWriteLocker		readDataLock(&m_dataLock);

			if(m_data.isEmpty()) {
				m_dataDescription.prepend(description);
				return;
			}

			data	=	QCryptographicHash::hash(m_data.takeFirst(), QCryptographicHash::Md5);
// 			data	=	m_data.takeFirst();
		}
		else if(description.first == 'p') {
			QWriteLocker		readDataLock(&m_dataPkgLock);

			if(m_dataPkg.isEmpty()) {
				m_dataDescription.prepend(description);
				return;
			}

			data	=	QCryptographicHash::hash(m_dataPkg.takeFirst(), QCryptographicHash::Md5);
// 			data	=	m_dataPkg.takeFirst();
		}
		else if(description.first == 's') {
			QWriteLocker		readDataLock(&m_dataFDLock);

			if(m_dataFD.isEmpty()) {
				m_dataDescription.prepend(description);
				return;
			}

			data	=	m_dataFD.takeFirst().toAscii();
		}


		readDescriptionLock.unlock();
		// m_dbgLog.write("Ok: 200\n");// m_dbgLog.flush();

		// m_dbgLog.write("ok\n");
		// m_dbgLog.flush();

// 		qDebug("=== got both ===");

// 		QWriteLocker		writeLocker(&m_peerStreamLock);
//
// 		if(description.first != data.first)
// 		{
// 			qDebug("Peer: PeerObject::checkReadData() - invalid type");
//
// // 			m_dbgLog.write("Invalid data type\n");
// // 			m_dbgLog.flush();
//
// 			m_peerSocket->write(QString("fWaited for %1 but received %2").arg(s_actionTitles[description.first]).arg(s_actionTitles[data.first]).toAscii());
// 			m_peerSocket->flush();
//
// 			if(m_close)
// 				close(1);
// 			return;
// 		}

		// Compare data
		if(data != description.second) {
			qDebug("Peer: PeerObject::checkReadData() - invalid data");

// 			m_dbgLog.write("Invalid data\n");
// 			m_dbgLog.flush();

			m_peerSocket->write("fReceived invalid data\n");
// 			m_peerSocket->flush();

			if(m_close)
				close(1);

			return;
		}

		static	quint32	totalRead	=	0;
		totalRead++;
		Logger::log("Processed", totalRead);

		m_peerSocket->write("s\n");
// 		m_peerSocket->flush();

		readDescriptionLock.relock();
	}

	if(m_close && m_dataDescription.isEmpty()) {
		if(m_data.isEmpty() && m_dataPkg.isEmpty() && m_dataFD.isEmpty())
			close();
		else {
			m_peerSocket->write("fNo more data descriptions\n");
// 			m_peerSocket->flush();

			close(1);
		}
	}
	else
		callSlotQueued(this, "checkReadData");

// 	m_dbgLog.write("PeerObject::~checkReadData()\n");
// 	m_dbgLog.flush();
}
