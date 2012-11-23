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

#ifndef TESTQUEUE_ENQUEUERUNNABLE_H
#define TESTQUEUE_ENQUEUERUNNABLE_H

#include <QtCore>

#include "../tsqueue.h"


template <class Q, class T>
class TestQueue_EnqueueRunnable : public QRunnable
{
	public:
		TestQueue_EnqueueRunnable(Q * queue, const QList<T>& data, QMutex * mutex = 0)
			:	m_queue(queue), m_data(data), m_mutex(mutex)
		{
			setAutoDelete(false);
			
			if(mutex)
				m_runFunc	=	&TestQueue_EnqueueRunnable::run_mutex;
			else
				m_runFunc	=	&TestQueue_EnqueueRunnable::run_pure;
		}
		
		
		virtual void run()
		{
			(this->*m_runFunc)();
		}
			
	private:
		void run_pure()
		{
			for(int i = 0; i < m_data.count(); i++)
				m_queue->enqueue(m_data[i]);
		}
		
		
		void run_mutex()
		{
			for(int i = 0; i < m_data.count(); i++)
			{
				m_mutex->lock();
				m_queue->enqueue(m_data[i]);
				m_mutex->unlock();
			}
		}
		
	private:
		void(TestQueue_EnqueueRunnable::*m_runFunc)();
		
		Q										*	m_queue;
		QList<T>							m_data;
		QMutex							*	m_mutex;
};

#endif // TESTQUEUE_ENQUEUERUNNABLE_H
