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

#if QT_VERSION >= 0x040000
	#include <QMutex>
	#define TsDataQueue_Mutex				QMutex
	#define TsDataQueue_ByteArray		QByteArray
	
	#define	MIN(a,b) qMin(a,b)
#else
	#error No mutex class found!
	#error No byte array class found!
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
			m_head	=	new TsDataQueueItem();
			m_tail	=	m_head;
			
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
			TsDataQueueItem	*	i	=	other.m_head;
			
			do
			{
				TsDataQueueItem	*	own	=	new TsDataQueueItem(*i);
				
				if(m_tail)
					m_tail->next	=	own;
				else
					m_head	=	m_tail	=	own;
			}while(i->next && (i = i->next));
		}
		
#ifdef UNIT_TEST
		void setUseGlobal(bool use)
		{
			m_useGlobalLock	=	use;
		}
#endif
		
		bool isEmpty() const
		{
			return m_tail->isEmpty();
		}
		
		uint size() const
		{
			uint	ret	=	0;
			
			// Lock dequeueing to prevent lower actual count of items
#ifdef UNIT_TEST
			if(m_useGlobalLock)
				m_locker.lock();
			else
#endif
			m_dequeueLocker.lock();
			
			TsDataQueueItem	*	i	=	m_head;
			
			// Find start
			while(i->isEmpty() && i->next)
				i	=	i->next;
			
			// Count
			if(!i->isEmpty())
			{
				do
				{
					ret	+=	i->m_data->size() - i->m_pos;
				}while(i->next && (i = i->next));
			}
			
#ifdef UNIT_TEST
			if(m_useGlobalLock)
				m_locker.unlock();
			else
#endif
			m_dequeueLocker.unlock();
			
			return ret;
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
#ifdef UNIT_TEST
			if(m_useGlobalLock)
				m_locker.lock();
			else
#endif
			m_enqueueLocker.lock();
			
			TsDataQueueItem	*	i	=	new TsDataQueueItem(data, size);
			
			m_tail->next	=	i;
			m_tail				=	i;
			
#ifdef UNIT_TEST
			if(m_useGlobalLock)
				m_locker.unlock();
			else
#endif
			m_enqueueLocker.unlock();
		}
		
		
		TsDataQueue_ByteArray peek(uint maxSize = 0) const
		{
			uint	dataSize	=	(maxSize == 0 ? size() : MIN(maxSize, size()));
			TsDataQueue_ByteArray	ret(dataSize, (char)0);
			
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
				
				memcpy(ret.data() + read, i->m_data->constData() + i->m_pos, tmp);
				read	+=	tmp;
				i	=	i->next;
			}
			
			ret.resize(read);
			
#ifdef UNIT_TEST
			if(m_useGlobalLock)
				m_locker.unlock();
			else
#endif
			m_dequeueLocker.unlock();
			
			return ret;
		}
		
		
		TsDataQueue_ByteArray dequeue(uint maxSize = 0)
		{
			uint	dataSize	=	(maxSize == 0 ? size() : MIN(maxSize, size()));
			TsDataQueue_ByteArray	ret(dataSize, (char)0);
			
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
				
				memcpy(ret.data() + read, m_head->m_data->constData() + m_head->m_pos, tmp);
				m_head->m_pos	+=	tmp;
				read	+=	tmp;
				
				if(m_head->isEmpty() && m_head->next)
				{
					TsDataQueueItem	*	next	=	m_head->next;
					delete m_head;
					m_head	=	next;
				}
			}
			
			ret.resize(read);
			
#ifdef UNIT_TEST
			if(m_useGlobalLock)
				m_locker.unlock();
			else
#endif
			m_dequeueLocker.unlock();
			
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
				
				if(m_head->isEmpty() && m_head->next)
				{
					TsDataQueueItem	*	next	=	m_head->next;
					delete m_head;
					m_head	=	next;
				}
			}
			
#ifdef UNIT_TEST
			if(m_useGlobalLock)
				m_locker.unlock();
			else
#endif
			m_dequeueLocker.unlock();
			
			return read;
		}
		
		
	private:
		mutable TsDataQueue_Mutex			m_enqueueLocker;
		mutable TsDataQueue_Mutex			m_dequeueLocker;
		
#ifdef UNIT_TEST
		mutable TsDataQueue_Mutex			m_locker;
		bool							m_useGlobalLock;
#endif
		
		TsDataQueueItem	*	m_head;
		TsDataQueueItem	*	m_tail;
};

#endif // TSDATAQUEUE_H
