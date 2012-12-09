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

#ifndef TESTQUEUE_DEQUEUERUNNABLE_H
#define TESTQUEUE_DEQUEUERUNNABLE_H

#include <QtCore>

#include "../tsqueue.h"


template <class Q, class T>
class TestQueue_DequeueRunnable : public QRunnable
{
	public:
		TestQueue_DequeueRunnable(Q * queue, int count, QMutex * mutex = 0)
			:	m_queue(queue), m_count(count), m_mutex(mutex)
		{
			setAutoDelete(false);
			
			if(mutex)
				m_runFunc	=	&TestQueue_DequeueRunnable::run_mutex;
			else
				m_runFunc	=	&TestQueue_DequeueRunnable::run_pure;
		}
		
		virtual void run()
		{
			(this->*m_runFunc)();
		}
		
	private:
		void run_pure()
		{
			for(int i = 0; i < m_count; i++)
				m_queue->dequeue();
		}
		
		
		void run_mutex()
		{
			for(int i = 0; i < m_count; i++)
			{
				m_mutex->lock();
				m_queue->dequeue();
				m_mutex->unlock();
			}
		}
		
	private:
		void(TestQueue_DequeueRunnable::*m_runFunc)();
		
		Q										*	m_queue;
		int										m_count;
		QMutex							*	m_mutex;
};

#endif // TESTQUEUE_DEQUEUERUNNABLE_H
