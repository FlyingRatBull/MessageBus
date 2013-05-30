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

#ifndef TSDATAQUEUE_H
#define TSDATAQUEUE_H

#include <string.h>

#include "global.h"


#if QT_VERSION >= 0x040000
	#include <QMutex>
	#include <QAtomicInt>
	#include <QWaitCondition>
	
	#define TsDataQueue_Mutex						QMutex
	#define TsDataQueue_ByteArray				QByteArray
	#define TsDataQueue_AtomicInt				QAtomicInt
	#define TsDataQueue_WaitCondition		QWaitCondition
	
	#define	MIN(a,b) qMin(a,b)
#else
	#error No mutex class found!
	#error No byte array class found!
	#error No atomic int class found!
	#error No wait condition class found!
#endif


class TsDataQueueItem
{
	public:
		inline TsDataQueueItem(const char * data, int size)
			:	next(0), m_data(new TsDataQueue_ByteArray(data, size)), m_pos(0)
		{
		}
		
		inline TsDataQueueItem(const TsDataQueueItem& other)
			:	next(0), m_data(0), m_pos(0)
		{
			if(other.m_data)
				m_data	=	new TsDataQueue_ByteArray(*(other.m_data));
			if(other.m_pos)
				m_pos		=	other.m_pos;
		}
		
		inline TsDataQueueItem()
			:	next(0), m_data(0), m_pos(0)
		{
		}
		
		inline ~TsDataQueueItem()
		{
			if(m_data)
			{
				delete m_data;
				m_data	=	0;
			}
		}
		
		inline void setEmpty()
		{
			delete m_data;
			m_data	=	0;
			m_pos		=	0;
		}
		
		inline bool isEmpty() const
		{
			return (m_data == 0 || m_pos >= m_data->size());
		}
		
		TsDataQueueItem						*	next;
		
		TsDataQueue_ByteArray			*	m_data;
		uint												m_pos;
		
};


class TsDataQueue
{
	public:
		TsDataQueue()
		{
			m_head			=	new TsDataQueueItem();
			m_tail			=	m_head;
			m_partSize	=	0;
			
#ifdef UNIT_TEST
			m_useGlobalLock	=	false;
#endif
		}
		
		TsDataQueue(const TsDataQueue &other)
		{
			*this	=	other;
			
#ifdef UNIT_TEST
			m_useGlobalLock	=	other.m_useGlobalLock;
#endif
		}
		
		virtual ~TsDataQueue()
		{
			clear();
			
			if(m_head)
				delete m_head;
		}
		
		virtual TsDataQueue &operator=(const TsDataQueue &other)
		{
			m_enqueueLocker.lock();
			other.m_enqueueLocker.lock();
			m_dequeueLocker.lock();
			other.m_dequeueLocker.lock();
			
			m_size							=	other.m_size;
			m_partSize					=	other.m_partSize;
			TsDataQueueItem	*	i	=	other.m_head;
			
			do
			{
				TsDataQueueItem	*	own	=	new TsDataQueueItem(*i);
				
				if(m_tail)
					m_tail->next	=	own;
				else
					m_head	=	m_tail	=	own;
			}while(i->next && (i = i->next));
			
			if(m_size > 0)
				m_nonEmpty.wakeAll();
			else
				m_empty.wakeAll();
			
			m_enqueueLocker.unlock();
			other.m_enqueueLocker.unlock();
			m_dequeueLocker.unlock();
			other.m_dequeueLocker.unlock();
			
			return *this;
		}
		
#ifdef UNIT_TEST
		void setUseGlobal(bool use)
		{
			m_useGlobalLock	=	use;
		}
#endif

		void setPartSize(uint size)
		{
			m_partSize	=	size;
		}
		
		bool isEmpty() const
		{
			return m_tail->isEmpty();
		}
		
		uint size() const
		{
			return (uint)m_size;
		
// 			uint	ret	=	0;
// 			
// 			// Lock dequeueing to prevent lower actual count of items
// #ifdef UNIT_TEST
// 			if(m_useGlobalLock)
// 				m_locker.lock();
// 			else
// #endif
// 			m_dequeueLocker.lock();
// 			
// 			TsDataQueueItem	*	i	=	m_head;
// 			
// 			// Find start
// 			while(i->isEmpty() && i->next)
// 				i	=	i->next;
// 			
// 			// Count
// 			if(!i->isEmpty())
// 			{
// 				do
// 				{
// 					ret	+=	i->m_data->size() - i->m_pos;
// 				}while(i->next && (i = i->next));
// 			}
// 			
// #ifdef UNIT_TEST
// 			if(m_useGlobalLock)
// 				m_locker.unlock();
// 			else
// #endif
// 			m_dequeueLocker.unlock();
// 			
// 			return ret;
		}

		
		void clear()
		{
			while(!isEmpty())
				dequeue();
		}
		
		
		void enqueue(const TsDataQueue_ByteArray& item)
		{
			enqueue(item.constData(), item.size());
		}
		
		
		void enqueue(const char * data, int size)
		{
			if(size <= 0)
				return;
			
#ifdef UNIT_TEST
			if(m_useGlobalLock)
				m_locker.lock();
			else
#endif
			m_enqueueLocker.lock();
			
			// Respect part size if needed
// 			if(m_partSize > 0)
// 			{
// 				int	pos		=	0;
// 				int	iSize	=	(m_tail->m_data ? m_tail->m_data->size() : 0);
// 				
// 				while(pos < size)
// 				{
// 					if(!m_tail->isEmpty() && iSize < m_partSize)
// 					{
// 						m_tail->m_data->append(data + pos, MIN(m_partSize - iSize - pos, quint32(size - pos)));
// 						pos	+=	(m_tail->m_data->size() - iSize);
// 						m_size.fetchAndAddOrdered(m_tail->m_data->size() - iSize);
// 						iSize	=	m_partSize;
// 					}
// 					else
// 					{
// 						TsDataQueueItem	*	i			=	new TsDataQueueItem(data + pos, MIN(quint32(size - pos), m_partSize));
// 						iSize	=		i->m_data->size();
// 						pos		+=	iSize;
// 						m_size.fetchAndAddOrdered(iSize);
// 						
// 						m_tail->next	=	i;
// 						m_tail				=	i;
// 					}
// 				}
// 			}
// 			// Ignore part size
// 			else
			{
				TsDataQueueItem	*	i	=	new TsDataQueueItem(data, size);
				m_tail->next	=	i;
				m_tail				=	i;
				m_size.fetchAndAddOrdered(size);
			}
			
			// We are not empty anymore
			m_nonEmpty.wakeAll();
			
#ifdef UNIT_TEST
			if(m_useGlobalLock)
				m_locker.unlock();
			else
#endif
			m_enqueueLocker.unlock();
		}
		
		
		TsDataQueue_ByteArray peek(uint maxSize = 0) const
		{
			uint	dataSize	=	(maxSize == 0 ? UINT_MAX : maxSize);
			TsDataQueue_ByteArray	ret;
			
			if(dataSize <= 8192)
				ret.reserve(dataSize);
			
#ifdef UNIT_TEST
			if(m_useGlobalLock)
				m_locker.lock();
			else
#endif
			m_dequeueLocker.lock();
			
			// Find beginning
			TsDataQueueItem	*	i	=	m_head;
			while(i->isEmpty() && i->next)
				i	=	i->next;
			
			uint	read	=	0;
			while(i && !i->isEmpty() && read < dataSize)
			{
				uint	tmp	=	MIN(i->m_data->size() - i->m_pos, dataSize - read);
				
				ret.resize(ret.size() + tmp);
				
				memcpy(ret.data() + read, i->m_data->constData() + i->m_pos, tmp);
				read	+=	tmp;
				i	=	i->next;
			}
			
// 			ret.resize(read);
			
#ifdef UNIT_TEST
			if(m_useGlobalLock)
				m_locker.unlock();
			else
#endif
			m_dequeueLocker.unlock();
			
			return ret;
		}
		
		
		uint dequeue(char * dest, uint maxSize)
		{
			uint	dataSize	=	(maxSize == 0 ? UINT_MAX : maxSize);
			
			#ifdef UNIT_TEST
			if(m_useGlobalLock)
				m_locker.lock();
			else
			#endif
			m_dequeueLocker.lock();
		
			// Delete empty items which are not at end
			while(m_head->isEmpty() && m_head->next)
			{
				TsDataQueueItem	*	next	=	m_head->next;
				delete m_head;
				m_head	=	next;
			}
			
			uint	read	=	0;
			while(!m_head->isEmpty() && read < dataSize)
			{
				uint	tmp	=	MIN(m_head->m_data->size() - m_head->m_pos, dataSize - read);
				
				memcpy(dest + read, m_head->m_data->constData() + m_head->m_pos, tmp);
				m_head->m_pos	+=	tmp;
				read	+=	tmp;
				m_size.fetchAndAddOrdered(-tmp);
				
				// Remove empty head
				if(m_head->isEmpty() && m_head->next)
				{
					TsDataQueueItem	*	next	=	m_head->next;
					delete m_head;
					m_head	=	next;
				}
			}
			
			if(m_size < 1)
				m_empty.wakeAll();
			
			#ifdef UNIT_TEST
			if(m_useGlobalLock)
				m_locker.unlock();
			else
			#endif
			m_dequeueLocker.unlock();
			
			return read;
		}
		
		
		TsDataQueue_ByteArray dequeue(uint maxSize = 0)
		{
			uint	dataSize	=	(maxSize == 0 ? size() : MIN(maxSize, size()));
			
			TsDataQueue_ByteArray	ret;
			ret.resize(dataSize);
			
			uint	len	=	dequeue(ret.data(), ret.size());
			
			if(len < ret.size())
				ret.resize(len);
			
			return ret;
		}
		
		
		uint discard(uint maxSize = 0)
		{
			uint	dataSize	=	(maxSize == 0 ? size() : MIN(maxSize, size()));
			
#ifdef UNIT_TEST
			if(m_useGlobalLock)
				m_locker.lock();
			else
#endif
			m_dequeueLocker.lock();
			
			// Delete empty items which are not at end
			while(m_head->isEmpty() && m_head->next)
			{
				TsDataQueueItem	*	next	=	m_head->next;
				delete m_head;
				m_head	=	next;
			}
			
			uint	read	=	0;
			while(!m_head->isEmpty() && read < dataSize)
			{
				uint	tmp	=	MIN(m_head->m_data->size() - m_head->m_pos, dataSize - read);
				
				m_head->m_pos	+=	tmp;
				read	+=	tmp;
				m_size.fetchAndAddOrdered(-tmp);
				
				// Remove empty head
				if(m_head->isEmpty() && m_head->next)
				{
					TsDataQueueItem	*	next	=	m_head->next;
					delete m_head;
					m_head	=	next;
				}
			}
			
			if(m_size < 1)
				m_empty.wakeAll();
			
#ifdef UNIT_TEST
			if(m_useGlobalLock)
				m_locker.unlock();
			else
#endif
			m_dequeueLocker.unlock();
			
			return read;
		}
		
		
		bool waitForNonEmpty(uint timeout)
		{
			m_enqueueLocker.lock();
			
			if(m_size > 0)
			{
				m_enqueueLocker.unlock();
				return true;
			}
			
			m_nonEmpty.wait(&m_enqueueLocker, timeout);
			m_enqueueLocker.unlock();
			
			return (m_size > 0);
		}
		
		
		void abortWaitForNonEmpty()
		{
			m_enqueueLocker.lock();
			
			m_nonEmpty.wakeAll();
			
			m_enqueueLocker.unlock();
		}
		
		
		bool waitForEmpty(uint timeout)
		{
			m_dequeueLocker.lock();
			
			if(m_size == 0)
			{
				m_dequeueLocker.unlock();
				return true;
			}
			
			m_empty.wait(&m_dequeueLocker, timeout);
			m_dequeueLocker.unlock();
			
			return (m_size < 1);
		}
		
		
		bool contains(char c) const
		{
			m_dequeueLocker.lock();
			TsDataQueueItem	*	i	=	m_head;
			
			while(i)
			{
				if(!i->isEmpty() && i->m_data->contains(c))
				{
					m_dequeueLocker.unlock();
					return true;
				}
				
				i	=	i->next;
			}
			
			m_dequeueLocker.unlock();
			
			return false;
		}
		
		
	private:
		mutable TsDataQueue_Mutex			m_enqueueLocker;
		mutable TsDataQueue_Mutex			m_dequeueLocker;
		TsDataQueue_WaitCondition		m_nonEmpty;
		TsDataQueue_WaitCondition		m_empty;
		
		// Size of each part
		uint													m_partSize;
		
		// Total size
		TsDataQueue_AtomicInt					m_size;
		
#ifdef UNIT_TEST
		mutable TsDataQueue_Mutex			m_locker;
		bool													m_useGlobalLock;
#endif
		
		TsDataQueueItem	*	m_head;
		TsDataQueueItem	*	m_tail;
};

#endif // TSDATAQUEUE_H
