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

#include "messagebus.h"

#include "localsocket.h"

// #include <sys/socket.h>
// #include <sys/un.h>

#include <QtCore>

#include "messagebus_p.h"

#include "tools.h"
#include "pointer.h"

class MSGBUS_LOCAL MessageBusPrivate
{
	public:
		MessageBusPrivate(const QString& service, const QString& objectName, QObject * object, MessageBus * parent)
		:	socket(new LocalSocket(parent)), p(parent), obj(object), m_dropNextSocketDescriptors(0), sName(socketName(service, objectName)), m_closed(false)
		{
// 				dbg("socket: 0x%08X", (uint)socket)
			m_returnValues		=	&m_retVals1;
			m_delReturnValues	=	&m_retVals2;
		}
		
		
		MessageBusPrivate(QObject * object, LocalSocket * sock, MessageBus * parent)
		:	socket(sock), p(parent), obj(object), m_dropNextSocketDescriptors(0), m_closed(false)
		{
			socket->setParent(parent);
			
			m_returnValues		=	&m_retVals1;
			m_delReturnValues	=	&m_retVals2;
		}


		~MessageBusPrivate()
		{
			close();
			LocalSocket	*	s	=	socket;
			socket	=	0;
			delete socket;
		}
		
		
		bool open()
		{
			bool	ret;
			
			if(sName.isEmpty())
				ret	=	socket->open(LocalSocket::ReadWrite);
			else
			{
				socket->connectToServer(sName, QIODevice::ReadWrite);
				ret	=	socket->waitForConnected();
			}
			
			if(!ret)
			{
				setError(MessageBus::SocketError, "Could not open socket!");
				qDebug("MessageBus: Could not open socket (%s)!", qPrintable(socket->errorString()));
			}
			
			return ret;
		}
		
		
		void close()
		{
			if(m_closed)
				return;
			
			m_closed	=	true;
			
			// Wake up all waiting calls
			{
				QReadLocker	readLock(&(m_returnValuesLock));
				foreach(quint64 id, m_returnValues->keys())
					m_returnValues->value(id)->isValAvailable.wakeAll();
				foreach(quint64 id, m_delReturnValues->keys())
					m_delReturnValues->value(id)->isValAvailable.wakeAll();
			}
			
			// Remove all return values ( Will be deleted automatically)
			{
				QWriteLocker	writeLock(&(m_returnValuesLock));
				m_returnValues->clear();
				m_delReturnValues->clear();
			}
			
			if(isValid())
				socket->close();
		}
		
		
		void setError(MessageBus::Error err, const QString& errorStr)
		{
			m_errorString	=	errorStr;
			emit(p->error(err));
			close();
		}

		
		void call(const QString& slot, qint64 timeout, const QList<Variant>& args)
		{
// 				dbg("d::call( ... )");
			__call(slot, timeout, args, CallSlot);
		}
		
		
		Variant callRet(const QString& slot, qint64 timeout, const QList<Variant>& args)
		{
// 				dbg("d::call( ... )");
			return __call(slot, timeout, args, CallSlotRet);
		}


		Variant __call(const QString& slot, qint64 timeout, const QList<Variant>& args, Command cmd)
		{
// 			qDebug("MessageBus::call - open: %s", socket->isOpen() ? "true" : "false");

			if(!isValid())
				return Variant();
			
			QElapsedTimer	elapsed;
			
			if(timeout > 0)
				elapsed.start();
			
			/// Unique call id
			int	callId	=	m_nextCallId.fetchAndAddRelaxed(1);

			QByteArray	pkg;
			QList<int>	socketDescriptors;
			quint32			tmp	=	0;

			char	c	=	(char)cmd;
			pkg.append(&c, sizeof(c));
			
			// We can safely asume that the size and byte order of an int on the other side is equal as we ony communicate with localhost
			pkg.append((const char*)(&callId), sizeof(callId));

			// Slot size
			QByteArray	sl(slot.toAscii());
			tmp	=	sl.size();
			pkg.append((const char*)&tmp, sizeof(tmp));

			// Slot
			pkg.append(sl);

			// Num args
			tmp	=	args.count();
			pkg.append((const char*)&tmp, sizeof(tmp));
			
// 				qDebug("... before args, size: %d", pkg.size());

			foreach(const Variant& var, args)
			{
// 					QByteArray	varData	=	writeVariant(var);
				
// 					qDebug("... arg size: %d", varData.size());
				
// 					pkg.append(varData);
				pkg.append(writeVariant(var));
				
				// Write socket descriptor
				if(var.type() == Variant::SocketDescriptor)
					socketDescriptors.append(var.toSocketDescriptor());
			}
			
			// Wait for call to be received if sending sockets
			bool	waitForRecv	=	false;
			if(!socketDescriptors.isEmpty() && cmd == CallSlot)
				waitForRecv	=	true;
				
// 			qDebug("[thread: 0x%08X] MessageBus::call(\"%s\", size: %d)", (int)QThread::currentThread(), sl.constData(), pkg.size());
			// Write socket descriptors first
// 			qDebug("[0x%08X] Writing socket descriptors ...", (int)this);
			foreach(int socketDescriptor, socketDescriptors)
			{
				// Check if socket is still open
				if(!isValid())
					return Variant();
				
				if(!isValid() || !socket->writeSocketDescriptor(socketDescriptor))
				{
					setError(MessageBus::TransferSocketDescriptorError, "Could not write socket descriptor!");
					qDebug("MessageBus: Could not write socket descriptor (%s)!", qPrintable(socket->errorString()));
					return Variant();
				}
			}
// 			qDebug("[0x%08X] ... done", (int)this);
			
			// Create return struct
			///@bug RetVal doesn't get deleted; Create unit test for Pointer<>
			Pointer<RetVal>	retVal(new RetVal);
			{
				retVal->callTransferred	=	false;
				
				// Safely insert return struct
				QWriteLocker	writeLock(&m_returnValuesLock);
				m_returnValues->insert(callId, retVal);
			}
			
// 			qDebug("[%p] Writing call package ...", this);
			// Write call package
			if(!isValid() || !socket->writePackage(pkg))
			{
				setError(MessageBus::TransferDataError, "Could not write package!");
				qDebug("MessageBus: Could not write package (%s)!", qPrintable(socket->errorString()));
				return Variant();
			}
// 			qDebug("[%p] ... done", this);
			
			if(waitForRecv)
			{
				// Lock for reading
				QReadLocker		readLocker(&retVal->lock);
				
				// Wait for call to be transferred
				if(!retVal->callTransferred)
					retVal->isCallTransferred.wait(&retVal->lock, (timeout > 0 ? timeout : m_callRetTimeout) - elapsed.elapsed());
				
				if(!retVal->callTransferred)
				{
					setError(MessageBus::WaitAnswerError, "Failed to wait for successful receival!");
					qDebug("MessageBus: Failed to wait for call to be transferred: %s", qPrintable(slot));
					return Variant();
				}
			}

			switch(cmd)
			{
				case CallSlot:
				{
					return Variant();
				}break;
					
				case CallSlotRet:
				{
					// Lock the return value for reading
					QReadLocker		readLocker(&retVal->lock);
					
					// Wait for return value to be available
					if(!retVal->val.isValid())
						retVal->isValAvailable.wait(&retVal->lock, (timeout > 0 ? timeout : m_callRetTimeout) - elapsed.elapsed());
					
					if(!retVal->val.isValid())
					{
						readLocker.unlock();
						// Remove return value
						{
							QWriteLocker	writeLock(&m_returnValuesLock);
							m_returnValues->remove(callId);
							m_delReturnValues->remove(callId);
						}
						
						setError(MessageBus::WaitAnswerError, "Failed to wait for return value!");
						qDebug("MessageBus: Failed to wait for return value!");
						return	Variant();
					}
					
					// Now we already have an return value
					
					Variant	ret(retVal->val);
					readLocker.unlock();
					
					// Remove return value
					{
						QWriteLocker	writeLock(&m_returnValuesLock);
						m_returnValues->remove(callId);
						m_delReturnValues->remove(callId);
					}
					
					return ret;
				}break;
			}
		}
		
		
		void readPackage(const QByteArray& package)
		{
			if(!socket)
				return;
			
// 			qDebug("MessageBus::runPackage()");
			
			if(package.isEmpty())
			{
				setError(MessageBus::TransferDataError, "Empty package received!");
				qDebug("MessageBus: Empty package received!");
				return;
			}
			
			quint32		dataPos	=	0;
			char			cmd			=	0;
			int				callId	=	0;
			
			// Read command
			memcpy(&cmd, package.constData() + dataPos, sizeof(cmd));
			dataPos	+=	sizeof(cmd);
			
			// Read call id
			memcpy(&callId, package.constData() + dataPos, sizeof(callId));
			dataPos	+=	sizeof(callId);
			
			switch(cmd)
			{
				case CallSlot:
				case CallSlotRet:
				{
					quint32	strLength	=	0;
					memcpy((char*)&strLength, package.constData() + dataPos, sizeof(strLength));
					dataPos	+=	sizeof(strLength);
					
					QString	str(package.mid(dataPos, strLength));
					dataPos	+=	strLength;
					
					// 					qDebug("[thread: 0x%08X] MessageBus::readData(\"%s\", size: %d)", (int) QThread::currentThread(), qPrintable(str), data.size());
					
					quint32	numArgs	=	0;
					memcpy((char*)&numArgs, package.constData() + dataPos, sizeof(numArgs));
					dataPos	+=	sizeof(numArgs);
					
					// 						qDebug("... before args, pos: %d", dataPos);
					
					str.prepend("0");
					str.append("(");
					QStringList	argList;
					for(qint64 i = 0; i < numArgs; i++)
						argList.append("Variant");
					str.append(argList.join(","));
					str.append(")");
					
					QByteArray	dat(str.toAscii());
					
					const char * slot	=	dat.constData();
					
					QList<Variant>	args;
					Variant					retVar;
					
					for(int i = 0; i < numArgs; i++)
					{
						// 							quint32	tmp	=	dataPos;
						// 							Variant	varData	=	readVariant(data, dataPos);
						
						// 							qDebug("... arg size: %d", (dataPos - tmp));
						// 							qDebug("... real arg size: %d", varData.toByteArray().size());
						
						// 							args.append(varData);
						args.append(readVariant(package, dataPos));
						
						// Check read socket descriptor
						if(args.last().type() == Variant::SocketDescriptor)
						{
							args.takeLast();
							
							QReadLocker	readLock(&m_pendingSocketDescriptorsLock);
						
							if(m_pendingSocketDescriptors.isEmpty())
								m_pendingSocketDescriptorsNonEmpty.wait(&m_pendingSocketDescriptorsLock, m_callRetTimeout);
							
							if(m_pendingSocketDescriptors.isEmpty())
							{
								setError(MessageBus::WaitAnswerError, "Failed to wait for socket descriptor!");
								m_dropNextSocketDescriptors++;
								qDebug("MessageBus: Failed to wait for socket descriptor!");
								return;
							}
							
							args.append(Variant::fromSocketDescriptor(m_pendingSocketDescriptors.takeFirst()));
						}
					}
					
					// Send call received command
					if(cmd == CallSlot || cmd == CallSlotRet)
					{
						QByteArray	pkg;
						quint32			tmp	=	0;
						
						char	c	=	(char)CallRecv;
						pkg.append(&c, 1);
						pkg.append((const char*)(&callId), sizeof(callId));
						
						// Check if socket is still open
						if(!isValid())
							return;
						
						// Should work too if run from another thread
						if(!socket->writePackage(pkg))
						{
							setError(MessageBus::TransferDataError, "Could not write successful receival package!");
							qDebug("MessageBus: Could not write received command!");
							return;
						}
							
					}
					
					if(dataPos < package.size())
					{
						setError(MessageBus::TransferDataError, "Package too long!");
						qDebug("MessageBus: Still data to read!");
						return;
					}
					
					bool ret	=	false;
					
					switch(args.count())
					{
						case 0:
						{
							if(cmd == CallSlot)
								ret	=	callSlotQueued(obj, slot, Q_ARG(MessageBus*, p));
							else
								ret	=	callSlotDirect(obj, slot, Q_ARG(MessageBus*, p), Q_ARG(Variant*, &retVar));
						}break;
						
						case 1:
						{
							if(cmd == CallSlot)
								ret	=	callSlotQueued(obj, slot, Q_ARG(MessageBus*, p), Q_ARG(Variant, args.at(0)));
							else
								ret	=	callSlotDirect(obj, slot, Q_ARG(MessageBus*, p), Q_ARG(Variant*, &retVar), Q_ARG(Variant, args.at(0)));
						}break;
						
						case 2:
						{
							if(cmd == CallSlot)
								ret	=	callSlotQueued(obj, slot, Q_ARG(MessageBus*, p), Q_ARG(Variant, args.at(0)), Q_ARG(Variant, args.at(1)));
							else
								ret	=	callSlotDirect(obj, slot, Q_ARG(MessageBus*, p), Q_ARG(Variant*, &retVar), Q_ARG(Variant, args.at(0)), Q_ARG(Variant, args.at(1)));
						}break;
						
						case 3:
						{
							if(cmd == CallSlot)
								ret	=	callSlotQueued(obj, slot, Q_ARG(MessageBus*, p), Q_ARG(Variant, args.at(0)), Q_ARG(Variant, args.at(1)), Q_ARG(Variant, args.at(2)));
							else
								ret	=	callSlotDirect(obj, slot, Q_ARG(MessageBus*, p), Q_ARG(Variant*, &retVar), Q_ARG(Variant, args.at(0)), Q_ARG(Variant, args.at(1)), Q_ARG(Variant, args.at(2)));
						}break;
						
						case 4:
						{
							if(cmd == CallSlot)
								ret	=	callSlotQueued(obj, slot, Q_ARG(MessageBus*, p), Q_ARG(Variant, args.at(0)), Q_ARG(Variant, args.at(1)), Q_ARG(Variant, args.at(2)), Q_ARG(Variant, args.at(3)));
							// 									else
								// 										ret	=	callSlotDirect(object, slot, Q_ARG(LocalSocket*, socket), Q_ARG(Variant*, &retVar), Q_ARG(Variant, args.at(0)), Q_ARG(Variant, args.at(1)), Q_ARG(Variant, args.at(2)), Q_ARG(Variant, args.at(3)));
						}break;
						
						// 								case 5:
						// 								{
							// 									if(cmd == CallSlot)
						// 										ret	=	callSlotAuto(object, slot, Q_ARG(LocalSocket*, socket), Q_ARG(Variant, args.at(0)), Q_ARG(Variant, args.at(1)), Q_ARG(Variant, args.at(2)), Q_ARG(Variant, args.at(3)), Q_ARG(Variant, args.at(4)));
						// 									else
						// 										ret	=	callSlotDirect(object, slot, Q_ARG(LocalSocket*, socket), Q_ARG(Variant*, &retVar), Q_ARG(Variant, args.at(0)), Q_ARG(Variant, args.at(1)), Q_ARG(Variant, args.at(2)), Q_ARG(Variant, args.at(3)));
						// 								}break;
						
						default:
							break;
						}
						
						if(cmd == CallSlotRet)
						{
							// 						qDebug("MessageBus: Returning value");
							
							// Write cmd
							QByteArray	pkg;
							quint32			tmp	=	0;
							
							char	c	=	(char)CallRetVal;
							pkg.append(&c, 1);
							pkg.append((const char*)(&callId), sizeof(callId));
							pkg.append(writeVariant(retVar));
							
							// Check if socket is still open
							if(!isValid())
								return;
							
							if(!socket->writePackage(pkg))
							{
								setError(MessageBus::TransferDataError, "Could not write return value!");
								qDebug("MessageBus: Could not write return value (%s)!", qPrintable(socket->errorString()));
								return;
							}
							else if(retVar.type() == Variant::SocketDescriptor)
								// Write socket descriptor
								socket->writeSocketDescriptor(retVar.toSocketDescriptor());
						}
						
						if(!ret)
							break;
					}break;
					
				case CallRetVal:
				{
					Variant	ret	=	readVariant(package, dataPos);
					
					// Read locker must be locked as long as we use retVal as retVal could be deleted otherwise
					QReadLocker	readLock(&m_returnValuesLock);
					
					Pointer<RetVal>	retVal(m_returnValues->value(callId));
					
					if(retVal == 0)
						retVal	=	m_delReturnValues->value(callId);
					
					// We don't have an registered ret value for this call id
					if(retVal == 0)
					{
						setError(MessageBus::TransferDataError, "Invalid package received!");
						qDebug("MessageBus: Return value with invalid call id received!");
						return;
					}
					
					readLock.unlock();
					
					QWriteLocker	writeLock(&retVal->lock);
					
					// We recevied an return value so the call was also transferred
					retVal->callTransferred	=	true;
					retVal->isCallTransferred.wakeAll();
					
					// Wait for socket descriptor if requested
					if(ret.type() == Variant::SocketDescriptor)
					{
						QReadLocker	readLock(&m_pendingSocketDescriptorsLock);
						
						if(m_pendingSocketDescriptors.isEmpty())
						{
							// We don't need the lock on the ret val while we wait on the socket descriptor
							writeLock.unlock();
							m_pendingSocketDescriptorsNonEmpty.wait(&m_pendingSocketDescriptorsLock, m_callRetTimeout);
							writeLock.relock();
						}
						
						if(m_pendingSocketDescriptors.isEmpty())
						{
							setError(MessageBus::WaitAnswerError, "Failed to wait for socket descriptor!");
							m_dropNextSocketDescriptors++;
							qDebug("MessageBus: Failed to wait for socket descriptor!");
							return;
						}
						
						ret.setValue(m_pendingSocketDescriptors.takeFirst());
					}
					
					retVal->val	=	ret;
					retVal->isValAvailable.wakeAll();
				}break;
					
				case CallRecv:
				{
					// Read locker can be unlocked directly as retVal is used within an managed pointer
					QReadLocker	readLock(&m_returnValuesLock);
					
					Pointer<RetVal>	retVal(m_returnValues->value(callId));
					
					if(retVal == 0)
						retVal	=	m_delReturnValues->value(callId);
					
					// We don't have an registered ret value for this call id
					if(retVal == 0)
					{
						setError(MessageBus::TransferDataError, "Invalid package received!");
						qDebug("MessageBus: Return value with invalid call id received!");
						return;
					}
					
					readLock.unlock();
					
					QWriteLocker	writeLock(&retVal->lock);
					
					retVal->callTransferred	=	true;
					retVal->isCallTransferred.wakeAll();
				}break;
			}
		}
		
		
		void readSocketDescriptor()
		{
			if(!socket)
				return;
			
			QWriteLocker	writeLock(&m_pendingSocketDescriptorsLock);
			int	socketDescriptor	=	socket->readSocketDescriptor();
			
			Q_ASSERT(socketDescriptor > 0);
			
			if(socketDescriptor < 1)
			{
				setError(MessageBus::TransferSocketDescriptorError, "Invalid socket descriptor received!");
				qDebug("MessageBus: Invalid socket descriptor received (%s)!", qPrintable(socket->errorString()));
				return;
			}
			
			if(m_dropNextSocketDescriptors)
				// Drop socket descriptor as it was requested earlier and we received it to late
				m_dropNextSocketDescriptors--;
			else
			{
				m_pendingSocketDescriptors.append(socketDescriptor);
				m_pendingSocketDescriptorsNonEmpty.wakeOne();
			}
		}


		inline bool isValid()
		{
			return socket && socket->isOpen();
		}
		
		
		void timerCheck()
		{
			// Delete timed out return values
			QWriteLocker		locker(&m_returnValuesLock);
			
			m_delReturnValues->clear();
			
			// Swap pointers
			QHash<int, Pointer<RetVal>	>	*	tmp	=	m_delReturnValues;
			m_delReturnValues	=	m_returnValues;
			m_returnValues	=	tmp;
		}
		
		
		void setCallRetTimeout(int ms)
		{
			m_callRetTimeout	=	ms;
			m_delReturnValuesTimer.setInterval(m_callRetTimeout);
		}


		/// Parent MessageBus object
		MessageBus					*	p;
		/// Object to receive calls
		QObject							*	obj;
		/// Socket for communication
		LocalSocket					*	socket;
		/// Last error string
		QString								m_errorString;
		/// Closed?
		bool									m_closed;
		
		/// Timeout of callRet
		int										m_callRetTimeout;
		
		/// Name of socket to connect to
		QString								sName;
		
		/// Next call id
		QAtomicInt						m_nextCallId;
		
		/// Return struct
		struct RetVal
		{
			/// Actucal return value
			Variant					val;
			bool						callTransferred;
			
			/// Mutex for locking val/callTransferred
			QMutex					mutex;
			/// Read/Write Lock for mutex
			QReadWriteLock	lock;
			/// Wait condition on val
			QWaitCondition	isValAvailable;
			
			/// Wait condition for received call
			QWaitCondition	isCallTransferred;
		};
		
		/// Return values by call id
		QHash<int, Pointer<RetVal>	>		m_retVals1;
		QHash<int, Pointer<RetVal>	>		m_retVals2;
		QHash<int, Pointer<RetVal>	>	*	m_returnValues;
		/// To be deleted return values
		QHash<int, Pointer<RetVal>	>	*	m_delReturnValues;
		/// Return values deletion timer
		QTimer													m_delReturnValuesTimer;
		/// Read/Write Lock for m_returnValues and m_delReturnValues
		QReadWriteLock				m_returnValuesLock;
		
		/// Received socket descriptors
		QList<int>						m_pendingSocketDescriptors;
		/// Mutex for m_pendingsocketDescriptors
		QMutex								m_pendingSocketDescriptorsMutex;
		/// Read/Write Lock for m_pendingSocketDescriptorsMutex
		QReadWriteLock				m_pendingSocketDescriptorsLock;
		/// m_pendingsocketDescriptorsMutex is non empty
		QWaitCondition				m_pendingSocketDescriptorsNonEmpty;
		/// Number of socket descriptors to drop upon arrival
		quint16								m_dropNextSocketDescriptors;
};


MessageBus::MessageBus(const QString& service, const QString& object, QObject * parent)
		:	QObject(parent), d(new MessageBusPrivate(service, object, parent, this))
{
	setCallRetTimeout(30000);
	connect(&d->m_delReturnValuesTimer, SIGNAL(timeout()), SLOT(timerCheck()));
	d->m_delReturnValuesTimer.start();
	
// 		connect(d->socket, SIGNAL(socketDescriptorAvailable()), this, SLOT(fetchSocketDescriptor()));
// 	connect(d->socket, SIGNAL(socketDescriptorReceived(int)), SIGNAL(socketDescriptorReceived(int)), Qt::DirectConnection);
	connect(d->socket, SIGNAL(readyReadPackage()), SLOT(onNewPackage()), Qt::DirectConnection);
	connect(d->socket, SIGNAL(readyReadSocketDescriptor()), SLOT(onNewSocketDescriptor()), Qt::DirectConnection);
	connect(d->socket, SIGNAL(disconnected()), SLOT(onDisconnected()), Qt::DirectConnection);

	if(!open())
		qWarning("MessageBus: Could not open socket to interface!");
	
// 		qDebug("MessageBus: open = %s", isOpen() ? "true" : "false");
}


MessageBus::MessageBus(QObject * target, LocalSocket * socket, QObject * parent)
		:	QObject(parent), d(new MessageBusPrivate(target, socket, this))
{
	setCallRetTimeout(30000);
	connect(&d->m_delReturnValuesTimer, SIGNAL(timeout()), SLOT(timerCheck()));
	d->m_delReturnValuesTimer.start();
	
// 	connect(d->socket, SIGNAL(socketDescriptorReceived(int)), this, SIGNAL(socketDescriptorReceived(int)), Qt::DirectConnection);
	connect(d->socket, SIGNAL(readyReadPackage()), SLOT(onNewPackage()), Qt::DirectConnection);
	connect(d->socket, SIGNAL(readyReadSocketDescriptor()), SLOT(onNewSocketDescriptor()), Qt::DirectConnection);
	connect(d->socket, SIGNAL(disconnected()), SLOT(onDisconnected()), Qt::DirectConnection);
	
	open();
}


MessageBus::~MessageBus()
{
	close();
	delete d;
}


void MessageBus::setCallRetTimeout(int ms)
{
	ms	=	qMax(ms, 1);
	d->setCallRetTimeout(ms);
}


Variant MessageBus::callRet(const QString& slot, const Variant& var1, const Variant& var2, const Variant& var3, const Variant& var4)
{
	if(!d->isValid())
		return Variant();
	
	return callRet(slot, 30000, var1, var2, var3, var4);
}


Variant MessageBus::callRet(const QString &slot, qint64 timeout, const Variant &var1, const Variant &var2, const Variant &var3, const Variant &var4)
{
	if(!d->isValid())
		return Variant();
	
	QList<Variant>	args;
	
	if(var1.isValid())
	{
		args	<<	var1;
		
		if(var2.isValid())
		{
			args	<<	var2;
			
			if(var3.isValid())
			{
				args	<<	var3;
				
				if(var4.isValid())
					args	<<	var4;
			}
		}
	}
	
	return callRet(slot, timeout, args);
}


Variant MessageBus::callRet(const QString &slot, const QList< Variant >& args)
{
	if(!d->isValid())
		return Variant();
	
	return callRet(slot, 30000, args);
}


Variant MessageBus::callRet(const QString &slot, qint64 timeout, const QList< Variant >& args)
{
	if(!d->isValid())
		return Variant();
	
	return d->callRet(slot, timeout, args);
}


void MessageBus::call(const QString& slot, const Variant& var1, const Variant& var2, const Variant& var3, const Variant& var4, const Variant& var5)
{
	if(!d->isValid())
		return;
	
	QList<Variant>	args;
	
	if(var1.isValid())
	{
		args	<<	var1;
		
		if(var2.isValid())
		{
			args	<<	var2;
			
			if(var3.isValid())
			{
				args	<<	var3;
				
				if(var4.isValid())
				{
					args	<<	var4;
					
					if(var5.isValid())
						args	<<	var5;
				}
			}
		}
	}
	
	call(slot, args);
}


void MessageBus::call(const QString &slot, const QList< Variant >& args)
{
	if(!d->isValid())
		return;
	
	d->call(slot, 0, args);
}



void MessageBus::onNewPackage()
{
	if(!d->isValid())
		return;
	
// 	qDebug("MessageBus::onNewPackage()");
	
	QtConcurrent::run(d, &MessageBusPrivate::readPackage, d->socket->readPackage());
}


void MessageBus::onNewSocketDescriptor()
{
	if(!d->isValid())
		return;
	
	QtConcurrent::run(d, &MessageBusPrivate::readSocketDescriptor);
}


void MessageBus::onDisconnected()
{
// 	qDebug("MessageBus::onDisconnected()");
	
	close();
	
	emit(disconnected());
}


void MessageBus::timerCheck()
{
	d->timerCheck();
}


bool MessageBus::open()
{
	if(d->isValid())
		return true;

	return d->open();
}


void MessageBus::close()
{
// 	qDebug("MessageBus::close()");
	d->close();
}


bool MessageBus::isOpen() const
{
	return d->isValid();
}


void MessageBus::setReceiver(QObject *obj)
{
	d->obj	=	obj;
}


QString MessageBus::errorString() const
{
	return d->m_errorString;
}


int __initMessageBus()
{
	qRegisterMetaType<MessageBus*>("MessageBus*");
	return 1;
}

Q_CONSTRUCTOR_FUNCTION(__initMessageBus)
