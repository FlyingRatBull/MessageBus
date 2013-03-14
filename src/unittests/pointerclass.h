#ifndef POINTERCLASS_H
#define POINTERCLASS_H

#include <QAtomicInt>

class PointerClass
{
	public:
		PointerClass(QAtomicInt& counter);
		
		~PointerClass();
		
		bool testFunc() const;
		
	private:
		QAtomicInt&			m_counter;
};

#endif // POINTERCLASS_H
