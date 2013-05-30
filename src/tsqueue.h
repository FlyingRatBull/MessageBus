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

#ifndef TSQUEUE_H
#define TSQUEUE_H

#include "global.h"

#if QT_VERSION >= 0x040000
	#include <qglobal.h>
	#include <QMutex>
	#include <QWaitCondition>
	
	#define TsQueue_Mutex					QMutex
	#define TsQueue_WaitCondition	QWaitCondition

	#define	ASSERT					Q_ASSERT
#else
	#error No mutex class found!
	#error No wait condition class found!
#endif

template <class T>
class TsQueueItem
{
	public:
		inline TsQueueItem(const T& data)
			:	next(0), m_data(new T(data))
		{
		}
		
		inline TsQueueItem(const TsQueueItem<T>& other)
			:	next(0), m_data(0)
		{
			if(other.m_data)
				m_data	=	new T(*(other.m_data));
		}
		
		inline TsQueueItem()
			:	next(0), m_data(0)
		{
		}
		
		inline ~TsQueueItem()
		{
			if(m_data)
				delete m_data;
		}
		
		inline void setEmpty()
		{
			delete m_data;
			m_data	=	0;
		}
		
		inline bool isEmpty() const
		{
			return (m_data == 0);
		}
		
		TsQueueItem<T>	*	next;
		
		T								*	m_data;
		
};

template <class T>
class TsQueue
{
	public:
		TsQueue()
		{
			m_head	=	new TsQueueItem<T>();
			m_tail	=	m_head;
			
#ifdef UNIT_TEST
			m_useGlobalLock	=	false;
#endif
		}
		
		TsQueue(const TsQueue<T> &other)
		{
			*this	=	other;
			
#ifdef UNIT_TEST
			m_useGlobalLock	=	other.m_useGlobalLock;
#endif
		}
		
		virtual ~TsQueue()
		{
			clear();
			
			if(m_head)
				delete m_head;
		}
		
		virtual TsQueue &operator=(const TsQueue &other)
		{
			TsQueueItem<T>	*	i	=	other.m_head;
			
			do
			{
				TsQueueItem<T>	*	own	=	new TsQueueItem<T>(*i);
				
				if(m_tail)
					m_tail->next	=	own;
				else
					m_head	=	m_tail	=	own;
			}while(i->next && (i = i->next));
			
			return *this;
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
		
		uint count() const
		{
			uint	ret	=	0;
			
			// Lock dequeueing to prevent lower actual count of items
#ifdef UNIT_TEST
			if(m_useGlobalLock)
				m_locker.lock();
			else
#endif
			m_dequeueLocker.lock();
			
			TsQueueItem<T>	*	i	=	m_head;
			
			// Find start
			while(i->isEmpty() && i->next)
				i	=	i->next;
			
			// Count
			if(!i->isEmpty())
			{
				do
				{
					ret++;
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
		
		const T& last() const
		{
			ASSERT(!m_tail->isEmpty());

			return *m_tail->m_data;
		}
		
		void clear()
		{
			while(!isEmpty())
				dequeue();
		}
		
		
		void enqueue(const T& item)
		{
#ifdef UNIT_TEST
			if(m_useGlobalLock)
				m_locker.lock();
			else
#endif
			m_enqueueLocker.lock();
			
			TsQueueItem<T>	*	i	=	new TsQueueItem<T>(item);
			
			m_tail->next	=	i;
			m_tail				=	i;
			
			m_nonEmpty.wakeAll();
			
#ifdef UNIT_TEST
			if(m_useGlobalLock)
				m_locker.unlock();
			else
#endif
			m_enqueueLocker.unlock();
		}
		
		
		T dequeue()
		{
			T	ret;
			
#ifdef UNIT_TEST
			if(m_useGlobalLock)
				m_locker.lock();
			else
#endif
			m_dequeueLocker.lock();
			
			// Delete empty items which are not at end
			while(m_head->isEmpty() && m_head->next)
			{
				TsQueueItem<T>	*	next	=	m_head->next;
				delete m_head;
				m_head	=	next;
			}
			
			if(!m_head->isEmpty())
			{
				ret	=	*m_head->m_data;
				m_head->setEmpty();
				
				if(m_head->next)
				{
					TsQueueItem<T>	*	next	=	m_head->next;
					delete m_head;
					m_head	=	next;
				}
			}
			
			if(isEmpty())
				m_empty.wakeAll();
			
#ifdef UNIT_TEST
			if(m_useGlobalLock)
				m_locker.unlock();
			else
#endif
			m_dequeueLocker.unlock();
			
			return ret;
		}
		
		
		bool waitForNonEmpty(uint timeout)
		{
			m_enqueueLocker.lock();
			
			if(!isEmpty())
			{
				m_enqueueLocker.unlock();
				return true;
			}
			
			m_nonEmpty.wait(&m_enqueueLocker, timeout);
			m_enqueueLocker.unlock();
			
			return !isEmpty();
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
			
			if(isEmpty())
			{
				m_dequeueLocker.unlock();
				return true;
			}
			
			m_empty.wait(&m_dequeueLocker, timeout);
			m_dequeueLocker.unlock();
			
			return isEmpty();
		}
		
		
	private:
		mutable TsQueue_Mutex		m_enqueueLocker;
		mutable TsQueue_Mutex		m_dequeueLocker;
		TsQueue_WaitCondition		m_nonEmpty;
		TsQueue_WaitCondition		m_empty;
		
#ifdef UNIT_TEST
		mutable TsQueue_Mutex			m_locker;
		bool											m_useGlobalLock;
#endif
		
		TsQueueItem<T>	*	m_head;
		TsQueueItem<T>	*	m_tail;
};

#endif // TSQUEUE_H
