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

#ifndef LOCALSOCKETPRIVATE_THREAD_H
#define LOCALSOCKETPRIVATE_THREAD_H

#include <QtCore/QThread>
#include <QtCore/QWaitCondition>

#include "variant.h"

#if defined Q_OS_LINUX || defined Q_OS_UNIX
	#include "implementations/localsocketprivate_unix.h"
#else
	#error No implementation of LocalSocket for this operating system!
#endif

class MSGBUS_LOCAL LocalSocketPrivate_Thread : public QThread
{
		Q_OBJECT
		
	public:
		LocalSocketPrivate_Thread(LocalSocket * q)
		{
			this->q	=	q;
			
			readBufferSize	=	0;
			
			reset();
		}
		
		
		void reset()
		{
			// readBufferSize is not effected by reset because it lasts over mutliplt connect() attempts
			lastError	=	LocalSocket::UnknownSocketError;
			state			=	LocalSocket::UnconnectedState;
			lsPrivate	=	0;
		}
		
		
		void close()
		{
			QThread::exit(0);
			
			QReadLocker	locker(&lsPrivateLock);
			
			if(lsPrivate)
				lsPrivateChanged.wait(&lsPrivateLock, 10000);
			
			if(lsPrivate)
			{
				locker.unlock();
				QThread::terminate();
				
				lsPrivateLock.lockForWrite();
				if(lsPrivate)
				{
					lsPrivate->deleteLater();
					lsPrivate	=	0;
				}
				lsPrivateLock.unlock();
			}
			
			reset();
		}
		
		
	protected:
		virtual void run()
		{
			{
				QWriteLocker		lsPrivateLocker(&lsPrivateLock);
#if defined(Q_OS_UNIX) || defined(Q_OS_LINX)
				lsPrivate		=	new LocalSocketPrivate_Unix(q);
#else
	#error No implementation of LocalSocket for this operating system!
#endif
				
				lsPrivate->setReadBufferSize(readBufferSize);
				
				// Direct connections
				connect(lsPrivate, SIGNAL(connected()), SLOT(onConnected()), Qt::DirectConnection);
				connect(lsPrivate, SIGNAL(disconnected()), SLOT(onDisconnected()), Qt::DirectConnection);
				
				connect(lsPrivate, SIGNAL(bytesWritten(qint64)), SLOT(onBytesWritten(qint64)), Qt::DirectConnection);
				connect(lsPrivate, SIGNAL(packageWritten()), SLOT(onPackageWritten()), Qt::DirectConnection);
				connect(lsPrivate, SIGNAL(socketDescriptorWritten(quintptr)), SLOT(onSocketDescriptorWritten(quintptr)), Qt::DirectConnection);
				
				connect(lsPrivate, SIGNAL(readyRead()), SLOT(onReadyRead()), Qt::DirectConnection);
				connect(lsPrivate, SIGNAL(readyReadPackage()), SLOT(onReadyReadPackage()), Qt::DirectConnection);
				connect(lsPrivate, SIGNAL(readyReadSocketDescriptor()), SLOT(onReadyReadSocketDescriptor()), Qt::DirectConnection);
				
				// Queued connections
				connect(lsPrivate, SIGNAL(error(LocalSocket::LocalSocketError,QString)), SLOT(onError(LocalSocket::LocalSocketError,QString)), Qt::QueuedConnection);
				
				lsPrivateChanged.wakeAll();
			}
			
// 			qDebug("--- exec()");
			
			QThread::exec();
			
// 			qDebug("--- ~exec()");
			
			// Wake all waiting threads
			QWriteLocker	locker1(&bytesWrittenLock);
			QWriteLocker	locker2(&packageWrittenLock);
			QWriteLocker	locker3(&socketDescriptorWrittenLock);
			QWriteLocker	locker4(&readyReadLock);
			QWriteLocker	locker5(&readyReadPackageLock);
			QWriteLocker	locker6(&readyReadSocketDescriptorLock);
			QWriteLocker	locker7(&stateLock);
			
			///@bug QLocalSocket closes on external peer side for unknown reasons. Maybe put breakpoint into QLocalSocket header
			
			areBytesWritten.wakeAll();
			isPackageWritten.wakeAll();
			isSocketDescriptorWritten.wakeAll();
			isReadyRead.wakeAll();
			isReadyReadPackage.wakeAll();
			isReadyReadSocketDescriptor.wakeAll();
			isStateChanged.wakeAll();
			
			QWriteLocker		lsPrivateLocker(&lsPrivateLock);
			lsPrivate->deleteLater();
			lsPrivate	=	0;
			lsPrivateChanged.wakeAll();
		}
		
	signals:
		void readyReadPackage();
		
		void readyReadSocketDescriptor();
		
		/*
		* Signals from QLocalSocket
		*/
		void connected();
		
		void disconnected();
		
		void error(LocalSocket::LocalSocketError socketError);
		
		void stateChanged(LocalSocket::LocalSocketState socketState);
		
		/*
		* Signals from QIODevice
		*/
		void bytesWritten(qint64 bytes);
		
		void packageWritten();
		
		void socketDescriptorWritten(quintptr);
		
		void readyRead();
		
	private slots:
		/*
		* WARNING Gets called from within the thread!
		*/
		void onConnected()
		{
// 			qDebug("onConnected()");
			
			{
				QWriteLocker		stateLocker(&stateLock);
				state	=	LocalSocket::ConnectedState;
				
				// Skip buffer of QIODevice
				q->QIODevice::open(lsPrivate->openMode() | QIODevice::Unbuffered);
				
				isStateChanged.wakeAll();
			}
			
			emit(stateChanged(state));
			emit(connected());
		}
		
		/*
		* WARNING Gets called from within the thread!
		*/
		void onDisconnected()
		{
// 			qDebug("onDisconnected()");
			
			{
				QWriteLocker		stateLocker(&stateLock);
				state	=	LocalSocket::UnconnectedState;
				
				q->QIODevice::close();
				
				isStateChanged.wakeAll();
			}
			
			emit(stateChanged(state));
			emit(disconnected());
			
			// Wake all threads waiting on data to become available as no more data can be received
			lsPrivate->m_readBuffer.abortWaitForNonEmpty();
		}
		
		/*
		* WARNING Gets called from within the thread!
		*/
		void onBytesWritten(qint64 bytes)
		{
			{
				QWriteLocker		bytesWrittenLocker(&bytesWrittenLock);
				areBytesWritten.wakeAll();
			}
			
			emit(bytesWritten(bytes));
		}
		
		/*
		* WARNING Gets called from within the thread!
		*/
		void onPackageWritten()
		{
			{
				QWriteLocker		packageWrittenLocker(&packageWrittenLock);
				isPackageWritten.wakeAll();
			}
			
			emit(packageWritten());
		}
		
		/*
		* WARNING Gets called from within the thread!
		*/
		void onSocketDescriptorWritten(quintptr socketDescriptor)
		{
			{
				QWriteLocker		socketDescriptorWrittenLocker(&socketDescriptorWrittenLock);
				isSocketDescriptorWritten.wakeAll();
			}
			
			emit(socketDescriptorWritten(socketDescriptor));
		}
		
		/*
		* WARNING Gets called from within the thread!
		*/
		void onReadyRead()
		{
			emit(readyRead());
			
			{
				QWriteLocker		readyReadLocker(&readyReadLock);
				isReadyRead.wakeAll();
			}
		}
		
		/*
		* WARNING Gets called from within the thread!
		*/
		void onReadyReadPackage()
		{
			emit(readyReadPackage());
			
			{
				QWriteLocker		readyReadPackageLocker(&readyReadPackageLock);
				isReadyReadPackage.wakeAll();
			}
		}
		
		/*
		* WARNING Gets called from within the thread!
		*/
		void onReadyReadSocketDescriptor()
		{
			emit(readyReadSocketDescriptor());
			
			{
				QWriteLocker		readyReadLocker(&readyReadSocketDescriptorLock);
				isReadyReadSocketDescriptor.wakeAll();
			}
		}
		
		void onError(LocalSocket::LocalSocketError err, const QString& errText)
		{
			lastError			=	err;
			
			q->setErrorString(errText);
			emit(error(err));
		}
		
	public:
		LocalSocket							*	q;
		
		/// Filename of socket we are connected to
		QString														serverFilename;
		
		/// Read buffer size
		qint64														readBufferSize;
		
		LocalSocket::LocalSocketError			lastError;
		
		/// Lock for areBytesWritten condition
		QReadWriteLock										bytesWrittenLock;
		/// Bytes have been written?
		QWaitCondition										areBytesWritten;
		/// Lock for isPackageWritten condition
		QReadWriteLock										packageWrittenLock;
		/// Package has been written?
		QWaitCondition										isPackageWritten;
		/// Lock for isSocketDescriptorWritten condition
		QReadWriteLock										socketDescriptorWrittenLock;
		/// Socket descriptor has been written?
		QWaitCondition										isSocketDescriptorWritten;
		
		/// Lock for isReadyRead condition
		QReadWriteLock										readyReadLock;
		/// readyRead() emitted?
		QWaitCondition										isReadyRead;
		/// Lock for isReadyReadPackage condition
		QReadWriteLock										readyReadPackageLock;
		/// readyReadPackage() emitted?
		QWaitCondition										isReadyReadPackage;
		/// Lock for isReadyReadSocketDescriptor condition
		QReadWriteLock										readyReadSocketDescriptorLock;
		/// readyReadSocketDescriptor() emitted?
		QWaitCondition										isReadyReadSocketDescriptor;
		
		/// Lock for state
		QReadWriteLock										stateLock;
		/// State changed?
		QWaitCondition										isStateChanged;
		LocalSocket::LocalSocketState			state;
		
		/// Lock for lsPrivate
		QReadWriteLock						lsPrivateLock;
		/// lsPrivate initialized?
		QWaitCondition						lsPrivateChanged;
		LocalSocketPrivate			*	lsPrivate;
};

#endif // LOCALSOCKETPRIVATE_THREAD
