#ifndef POINTER_H
#define POINTER_H

#include <QAtomicInt>
#include <QReadWriteLock>
#include <QHash>

template <typename T>
class Pointer
{
	public:
		Pointer(T * value = 0, bool autoDelete = true)
			:	m_value(0), m_counter(0), m_autoDelete(autoDelete)
		{
			registerValue(value);
		}
		
		Pointer(const Pointer<T>& other)
			:	m_value(0), m_counter(0), m_autoDelete(false)
		{
			*this	=	other;
		}
		
		virtual ~Pointer()
		{
			unregisterValue();
		}
		
		virtual Pointer& operator=(const Pointer<T>& other)
		{
			if(*this == other)
				return *this;
			
			unregisterValue();
			
			if(other.m_value == 0)
				return *this;
			
			m_value				=	other.m_value;
			m_counter			=	other.m_counter;
			m_autoDelete	=	other.m_autoDelete;
			
			m_counter->fetchAndAddOrdered(1);
			
			return *this;
		}
		
		virtual Pointer& operator=(T * other)
		{
			if(*this == other)
				return *this;
			
			unregisterValue();
			
			if(other == 0)
				return *this;
			
			registerValue(other);
			
			return *this;
		}
		
		virtual bool operator==(const Pointer<T>& other) const
		{
			return (m_value == other.m_value);
		}
		
		virtual bool operator!=(const Pointer<T>& other) const
		{
			return (m_value != other.m_value);
		}
		
		virtual bool operator==(const T* other) const
		{
			return (m_value == other);
		}
		
		virtual bool operator!=(const T* other) const
		{
			return (m_value != other);
		}
		
		virtual T* operator->() const
		{
			return m_value;
		}
		
		virtual T* value() const
		{
			return m_value;
		}
		
		void setAutoDelete(bool enabled = true)
		{
			m_autoDelete	=	enabled;
		}
		
	private:
		void registerValue(T * newValue)
		{
			unregisterValue();
			
			if(newValue == 0)
				return;
			
			// Get counter
			s_ValueCounterLock.lockForRead();
			m_counter	=	s_valueCounter.value(newValue, 0);
			s_ValueCounterLock.unlock();
			
			// No counter yet -> register new counter
			if(m_counter == 0)
			{
				s_ValueCounterLock.lockForWrite();
				
				// A new counter could be created in the meantime -> recheck
				m_counter	=	s_valueCounter.value(newValue, 0);
				
				if(!m_counter)
				{
					m_counter	=	new QAtomicInt();
					s_valueCounter.insert(newValue, m_counter);
				}
				
				s_ValueCounterLock.unlock();
			}
			
			// Raise counter
			m_counter->fetchAndAddOrdered(1);
			
			// Set value
			m_value	=	newValue;
		}
		
		void unregisterValue()
		{
			if(m_value == 0)
				return;
			
			if(m_counter->fetchAndAddOrdered(-1) == 1)
			{
				s_ValueCounterLock.lockForWrite();
// 				delete s_valueCounter.take(m_value);
				s_valueCounter.remove(m_value);
				delete m_counter;
				s_ValueCounterLock.unlock();
				
				if(m_autoDelete)
					delete m_value;
			}
			
			m_counter	=	0;
			m_value		=	0;
		}
		
	private:
		T							*	m_value;
		QAtomicInt		*	m_counter;
		
		bool						m_autoDelete;
		
	private:
		static	QHash<T*, QAtomicInt*>		s_valueCounter;
		static	QReadWriteLock						s_ValueCounterLock;
};

template <typename T>	QHash<T*, QAtomicInt*>		Pointer<T>::s_valueCounter;
template <typename T>	QReadWriteLock						Pointer<T>::s_ValueCounterLock;

#endif // POINTER_H
