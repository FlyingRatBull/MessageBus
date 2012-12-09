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
		:	socket(new LocalSocket(socketName(service, objectName), parent)), p(parent), obj(object), m_safeCallRetReceived(false)
		{
// 				dbg("socket: 0x%08X", (uint)socket)
		}
		
		
		MessageBusPrivate(QObject * object, LocalSocket * sock, MessageBus * parent)
				:	socket(sock), p(parent), obj(object)
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

		
		void call(const QString& slot, const QList<Variant>& args)
		{
// 				dbg("d::call( ... )");
			__call(slot, args, CallSlot);
		}
		
		
		Variant callRet(const QString& slot, const QList<Variant>& args)
		{
// 				dbg("d::call( ... )");
			return __call(slot, args, CallSlotRet);
		}


		Variant __call(const QString& slot, const QList<Variant>& args, Command cmd)
		{
// 			qDebug("MessageBus::call - open: %s", socket->isOpen() ? "true" : "false");

			if(!socket->isOpen())
				return Variant();

			QByteArray	pkg;
			QList<int>	socketDescriptors;
			quint32			tmp	=	0;

			char	c	=	(char)cmd;
			pkg.append(&c, sizeof(c));

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
			
			// Use safe call if sending sockets
			if(!socketDescriptors.isEmpty() && cmd == CallSlot)
			{
				cmd	=	SafeCall;
				c		=	(char)cmd;
				pkg.replace(0, 1, &c, 1);
			}
				
// 			qDebug("[thread: 0x%08X] MessageBus::call(\"%s\", size: %d)", (int)QThread::currentThread(), sl.constData(), pkg.size());
			// Write socket descriptors first
// 			qDebug("[0x%08X] Writing socket descriptors ...", (int)this);
			foreach(int socketDescriptor, socketDescriptors)
				socket->writeSocketDescriptor(socketDescriptor);
// 			qDebug("[0x%08X] ... done", (int)this);

			// Write call package
			// Lock for safe call
			if(cmd == SafeCall)
				m_safeCallRetReceived	=	false;
			
// 			qDebug("[0x%08X] Writing call package ...", (int)this);
			socket->writePackage(pkg);
// 			qDebug("[0x%08X] ... done", (int)this);

			switch(cmd)
			{
				case CallSlot:
					return Variant();break;
					
				case CallSlotRet:
				{
					///@todo Adopt double locking behaviour from SafeCall
					QMutexLocker	locker(&m_answerMutex);
					m_answerWait.wait(&m_answerMutex, 30000);
					
					if(m_retVals.isEmpty())
					{
						qWarning("MessageBus: Failed to wait for answer for: %s", qPrintable(slot));
						return Variant();
					}
					
					QMutexLocker	lock(&m_retValsMutex);
					
					// Check read socket descriptor
					if(m_retVals.first().type() == Variant::SocketDescriptor)
					{
						m_retVals.takeFirst();
						m_retVals.prepend(Variant::fromSocketDescriptor(socket->readSocketDescriptor()));
					}
					
					return m_retVals.takeFirst();
				}break;
				
				case SafeCall:
				{
					
					
					// Check if we have to wait for the return value
// 					qDebug("[0x%08X] Waiting for safe call return ...", (int)this);
					
					m_safeCallSafeMutex.lock();
					if(!m_safeCallRetReceived)
					{
						QMutexLocker	locker(&m_safeCallMutex);
						m_safeCallSafeMutex.unlock();
						m_safeCallWait.wait(&m_safeCallMutex, 30000);
					}
					else
						m_safeCallSafeMutex.unlock();
					
// 					qDebug("[0x%08X] ... done", (int)this);
					
					return Variant();
				}break;
			}
		}
		
		
		void readData()
		{
			if(QThread::currentThread() == p->thread())
				qFatal("Called from same thread!");
			
			QByteArray	data(socket->readPackage());
			
			if(data.isEmpty())
				return;
			
			quint32		dataPos	=	0;
			char			cmd			=	0;
			
			memcpy(&cmd, data.constData() + dataPos, sizeof(cmd));
			dataPos	+=	sizeof(cmd);
			
			switch(cmd)
			{
				case SafeCall:
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
					
					if(dataPos < data.size())
						qDebug("MessageBus: Still data to read!");

					bool ret	=	false;

					switch(args.count())
					{
						case 0:
						{
							if(cmd == CallSlot || cmd == SafeCall)
								ret	=	callSlotQueued(obj, slot, Q_ARG(MessageBus*, p));
							else
								ret	=	callSlotDirect(obj, slot, Q_ARG(MessageBus*, p), Q_ARG(Variant*, &retVar));
						}break;

						case 1:
						{
							if(cmd == CallSlot || cmd == SafeCall)
								ret	=	callSlotQueued(obj, slot, Q_ARG(MessageBus*, p), Q_ARG(Variant, args.at(0)));
							else
								ret	=	callSlotDirect(obj, slot, Q_ARG(MessageBus*, p), Q_ARG(Variant*, &retVar), Q_ARG(Variant, args.at(0)));
						}break;

						case 2:
						{
							if(cmd == CallSlot || cmd == SafeCall)
								ret	=	callSlotQueued(obj, slot, Q_ARG(MessageBus*, p), Q_ARG(Variant, args.at(0)), Q_ARG(Variant, args.at(1)));
							else
								ret	=	callSlotDirect(obj, slot, Q_ARG(MessageBus*, p), Q_ARG(Variant*, &retVar), Q_ARG(Variant, args.at(0)), Q_ARG(Variant, args.at(1)));
						}break;

						case 3:
						{
							if(cmd == CallSlot || cmd == SafeCall)
								ret	=	callSlotQueued(obj, slot, Q_ARG(MessageBus*, p), Q_ARG(Variant, args.at(0)), Q_ARG(Variant, args.at(1)), Q_ARG(Variant, args.at(2)));
							else
								ret	=	callSlotDirect(obj, slot, Q_ARG(MessageBus*, p), Q_ARG(Variant*, &retVar), Q_ARG(Variant, args.at(0)), Q_ARG(Variant, args.at(1)), Q_ARG(Variant, args.at(2)));
						}break;

						case 4:
						{
							if(cmd == CallSlot || cmd == SafeCall)
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

					if(cmd == SafeCall)
					{
// 						qDebug("[0x%08X] Writing safe call return ...", (int)this);
						
						QByteArray	pkg;
						quint32			tmp	=	0;

						char	c	=	(char)SafeCallRet;
						pkg.append(&c, 1);
						
						if(!socket->writePackage(pkg))
							qWarning("MessageBus: Could not write return value!");
						
// 						qDebug("[0x%08X] ... done", (int)this);
					}
					else if(cmd == CallSlotRet)
					{
// 							qDebug("MessageBus: Returning value");
						// Write cmd
						QByteArray	pkg;
						quint32			tmp	=	0;

						char	c	=	(char)CallRetVal;
						pkg.append(&c, 1);
						pkg.append(writeVariant(retVar));
						
						if(!socket->writePackage(pkg))
							qWarning("MessageBus: Could not write return value!");
						else if(retVar.type() == Variant::SocketDescriptor)
							// Write socket descriptor
							socket->writeSocketDescriptor(retVar.toSocketDescriptor());
						
// 							qDebug("MessageBus: Value returned");
					}

					if(!ret)
						break;
				}break;
				
				case CallRetVal:
				{
// 					qDebug("MessageBus: Got return value");
					
					Variant	ret	=	readVariant(data, dataPos);
					
					m_retValsMutex.lock();
					m_retVals.append(ret);
					m_retValsMutex.unlock();
					
// 					qDebug("MessageBus: waking");
					m_answerWait.wakeOne();
// 					qDebug("MessageBus: waking: ok");
				}break;
				
				case SafeCallRet:
				{
					{
						QMutexLocker	locker(&m_safeCallSafeMutex);
						m_safeCallRetReceived	=	true;
					}
					
// 					qDebug("[0x%08X] Received safe call return", (int)this);
					
// 					qDebug("MessageBus: waking");
					m_safeCallWait.wakeOne();
// 					qDebug("MessageBus: waking: ok");
				}break;
				
// 				case Data:
// 				{
// 					quint32	size	=	0;
// 					socket->read((char*)&size, sizeof(quint32));
// 					
// 					m_bufferMutex.lock();
// 					m_buffer.append(socket->read(size));
// 					m_bufferMutex.unlock();
// 					
// 					emit(p->readyRead());
// 				}break;
			}
		}


		inline bool isValid()
		{
			return socket->isOpen();
		}


		MessageBus							*	p;
		QObject							*	obj;
		LocalSocket					*	socket;

		QList<Variant>				m_retVals;
		QMutex								m_retValsMutex;
// 		QByteArray						m_buffer;
// 		QMutex								m_bufferMutex;
		
		QMutex								m_answerMutex;
		QWaitCondition				m_answerWait;
		
		QMutex								m_safeCallMutex;
		QMutex								m_safeCallSafeMutex;
		QWaitCondition				m_safeCallWait;
		bool									m_safeCallRetReceived;

// 			struct	sockaddr_un		unix_socket_name;
};


MessageBus::MessageBus(const QString& service, const QString& object, QObject * parent)
		:	QObject(parent), d(new MessageBusPrivate(service, object, parent, this))
{
// 		connect(d->socket, SIGNAL(socketDescriptorAvailable()), this, SLOT(fetchSocketDescriptor()));
// 	connect(d->socket, SIGNAL(socketDescriptorReceived(int)), SIGNAL(socketDescriptorReceived(int)), Qt::DirectConnection);
	connect(d->socket, SIGNAL(readyReadPackage()), SLOT(onNewPackage()), Qt::DirectConnection);
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
	connect(d->socket, SIGNAL(disconnected()), SLOT(onDisconnected()), Qt::DirectConnection);
	
	open();
}


MessageBus::~MessageBus()
{
	delete d;
}


Variant MessageBus::callRet(const QString& slot, const Variant& var1, const Variant& var2, const Variant& var3, const Variant& var4)
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

	return d->callRet(slot, args);
}


Variant MessageBus::callRet(const QString &slot, const QList< Variant >& args)
{
	if(!d->isValid())
		return Variant();
	
	return d->callRet(slot, args);
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
					args	<<	var4;
			}
		}
	}

	d->call(slot, args);
}


void MessageBus::call(const QString &slot, const QList< Variant >& args)
{
	if(!d->isValid())
		return;
	
	d->call(slot, args);
}



void MessageBus::onNewPackage()
{
	if(!d->isValid())
		return;
	
	d->readData();
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
	
	if(d->isValid())
		d->socket->close();
}


bool MessageBus::isOpen() const
{
	return d->isValid();
}


int __initMessageBus()
{
	qRegisterMetaType<MessageBus*>("MessageBus*");
	return 1;
}

Q_CONSTRUCTOR_FUNCTION(__initMessageBus)
