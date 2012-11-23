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

#ifndef TSQUEUE_H
#define TSQUEUE_H

#if QT_VERSION >= 0x040000
	#include <QMutex>
	#define TsQueue_Mutex		QMutex
#else
	#error No mutex class found!
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
			if(m_tail->isEmpty())
				return T();
			else
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
			
#ifdef UNIT_TEST
			if(m_useGlobalLock)
				m_locker.unlock();
			else
#endif
			m_dequeueLocker.unlock();
			
			return ret;
		}
		
		
	private:
		mutable TsQueue_Mutex			m_enqueueLocker;
		mutable TsQueue_Mutex			m_dequeueLocker;
		
#ifdef UNIT_TEST
		mutable TsQueue_Mutex			m_locker;
		bool							m_useGlobalLock;
#endif
		
		TsQueueItem<T>	*	m_head;
		TsQueueItem<T>	*	m_tail;
};

#endif // TSQUEUE_H
