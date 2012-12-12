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

class MessageBusPrivate
{
	public:
		MessageBusPrivate(const QString& service, const QString& objectName, QObject * object, MessageBus * parent)
		:	socket(new LocalSocket(socketName(service, objectName), parent)), p(parent), obj(object), m_dropNextSocketDescriptors(0)
		{
// 				dbg("socket: 0x%08X", (uint)socket)
		}
		
		
		MessageBusPrivate(QObject * object, LocalSocket * sock, MessageBus * parent)
		:	socket(sock), p(parent), obj(object), m_dropNextSocketDescriptors(0)
		{
			socket->setParent(parent);
		}
		
		
		bool open()
		{
			return socket->open(LocalSocket::ReadWrite);
		}


		~MessageBusPrivate()
		{
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

			if(!socket->isOpen())
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
				socket->writeSocketDescriptor(socketDescriptor);
// 			qDebug("[0x%08X] ... done", (int)this);
			
			// Create return struct
			RetVal	*	retVal	=	new RetVal;
			{
				retVal->callTransferred	=	false;
				
				// Safely insert return struct
				QWriteLocker	readLock(&m_returnValuesLock);
				m_returnValues[callId]	=	retVal;
			}
			
// 			qDebug("[0x%08X] Writing call package ...", (int)this);
			// Write call package
			socket->writePackage(pkg);
// 			qDebug("[0x%08X] ... done", (int)this);
			
			if(waitForRecv)
			{
				// Lock for reading
				QReadLocker		readLocker(&retVal->lock);
				
				// Wait for call to be transferred
				if(!retVal->callTransferred)
					retVal->isCallTransferred.wait(&retVal->lock, (timeout > 0 ? timeout - elapsed.elapsed() : 30000));
				
				if(!retVal->callTransferred)
				{
					qWarning("MessageBus: Failed to wait for call to be transferred: %s", qPrintable(slot));
					return Variant();
				}
				
				readLocker.unlock();
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
						retVal->isValAvailable.wait(&retVal->lock, (timeout > 0 ? timeout - elapsed.elapsed() : 30000));
					
					if(!retVal->val.isValid())
						return Variant();
					
					// Now we already have an return value
					
					Variant	ret(retVal->val);
					readLocker.unlock();
					
					// Remove return value
					{
						QWriteLocker	readLock(&m_returnValuesLock);
						m_returnValues.remove(callId);
					}
					
					delete retVal;
					
					return ret;
				}break;
			}
		}
		
		
		void readData()
		{
// 			qDebug("MessageBus::readData()");
			
			if(QThread::currentThread() == p->thread())
				qFatal("Called from same thread!");
			
			QByteArray	data(socket->readPackage());
			
			if(data.isEmpty())
				return;
			
			quint32		dataPos	=	0;
			char			cmd			=	0;
			int				callId	=	0;
			
			// Read command
			memcpy(&cmd, data.constData() + dataPos, sizeof(cmd));
			dataPos	+=	sizeof(cmd);
			
			// Read call id
			memcpy(&callId, data.constData() + dataPos, sizeof(callId));
			dataPos	+=	sizeof(callId);
			
			switch(cmd)
			{
				case CallSlot:
				case CallSlotRet:
				{
					quint32	strLength	=	0;
					memcpy((char*)&strLength, data.constData() + dataPos, sizeof(strLength));
					dataPos	+=	sizeof(strLength);
					
					QString	str(data.mid(dataPos, strLength));
					dataPos	+=	strLength;
					
// 					qDebug("[thread: 0x%08X] MessageBus::readData(\"%s\", size: %d)", (int) QThread::currentThread(), qPrintable(str), data.size());
					
					quint32	numArgs	=	0;
					memcpy((char*)&numArgs, data.constData() + dataPos, sizeof(numArgs));
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
						args.append(readVariant(data, dataPos));
						
						// Check read socket descriptor
						if(args.last().type() == Variant::SocketDescriptor)
						{
							args.takeLast();
							args.append(Variant::fromSocketDescriptor(socket->readSocketDescriptor()));
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
						
						if(!socket->writePackage(pkg))
							qWarning("MessageBus: Could not write received command!");
					}
					
					if(dataPos < data.size())
						qDebug("MessageBus: Still data to read!");

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
						
						if(!socket->writePackage(pkg))
							qWarning("MessageBus: Could not write return value!");
						else if(retVar.type() == Variant::SocketDescriptor)
							// Write socket descriptor
							socket->writeSocketDescriptor(retVar.toSocketDescriptor());
					}

					if(!ret)
						break;
				}break;
				
				case CallRetVal:
				{
					Variant	ret	=	readVariant(data, dataPos);
					
					RetVal	*	retVal	=	0;
					{
						QReadLocker	readLock(&m_returnValuesLock);
						retVal	=	m_returnValues[callId];
					}
					
					// We don't have an registered ret value for this call id
					if(retVal == 0)
					{
						qWarning("Return value with invalid call id received!");
						break;
					}
					
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
							m_pendingSocketDescriptorsNonEmpty.wait(&m_pendingSocketDescriptorsLock, 30000);
							writeLock.relock();
						}
						
						if(m_pendingSocketDescriptors.isEmpty())
						{
							m_dropNextSocketDescriptors++;
							qWarning("No socket descriptor recevied for return value of type SocketDescriptor!");
							break;
						}
						
						ret.setValue(m_pendingSocketDescriptors.takeFirst());
					}
					
					retVal->val	=	ret;
					retVal->isValAvailable.wakeAll();
				}break;
				
				case CallRecv:
				{
					RetVal	*	retVal	=	0;
					{
						QReadLocker	readLock(&m_returnValuesLock);
						retVal	=	m_returnValues[callId];
					}
					
					// We don't have an registered ret value for this call id
					if(retVal == 0)
					{
						qWarning("Return value with invalid call id received!");
						break;
					}
					
					QWriteLocker	writeLock(&retVal->lock);
					
					retVal->callTransferred	=	true;
					retVal->isCallTransferred.wakeAll();
				}break;
			}
		}
		
		
		void readSocketDescriptor()
		{
			QWriteLocker	writeLock(&m_pendingSocketDescriptorsLock);
			int	socketDescriptor	=	socket->readSocketDescriptor();
			
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
			return socket->isOpen();
		}


		/// Parent MessageBus object
		MessageBus					*	p;
		/// Object to receive calls
		QObject							*	obj;
		/// Socket for communication
		LocalSocket					*	socket;
		
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
		QHash<int, RetVal*>		m_returnValues;
		/// Mutex for m_returnValues
		QMutex								m_returnValuesMutex;
		/// Read/Write Lock for m_returnValuesMutex
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
	
	d->readData();
}


void MessageBus::onNewSocketDescriptor()
{
	if(!d->isValid())
		return;
	
	d->readSocketDescriptor();
}


void MessageBus::onDisconnected()
{
// 	qDebug("MessageBus::onDisconnected()");
	
	close();
	
	emit(disconnected());
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
	// Wake up all waiting calls
	{
		QReadLocker	readLock(&(d->m_returnValuesLock));
		foreach(quint64 id, d->m_returnValues.keys())
			d->m_returnValues[id]->isValAvailable.wakeAll();
	}
	
	// Remove all return values
	{
		QWriteLocker	writeLock(&(d->m_returnValuesLock));
		foreach(quint64 id, d->m_returnValues.keys())
			delete d->m_returnValues[id];
		d->m_returnValues.clear();
	}
	
	if(d->isValid())
		d->socket->close();
}


bool MessageBus::isOpen() const
{
	return d->isValid();
}


void MessageBus::setReceiver(QObject *obj)
{
	d->obj	=	obj;
}


int __initMessageBus()
{
	qRegisterMetaType<MessageBus*>("MessageBus*");
	return 1;
}

Q_CONSTRUCTOR_FUNCTION(__initMessageBus)
