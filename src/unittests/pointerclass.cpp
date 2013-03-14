#include "pointerclass.h"

PointerClass::PointerClass(QAtomicInt& counter)
	:	m_counter(counter)
{
// 	qDebug("PointerClass()");
	m_counter.fetchAndAddOrdered(1);
}


PointerClass::~PointerClass()
{
	m_counter.fetchAndAddOrdered(-1);
// 	qDebug("~PointerClass()");
}


bool PointerClass::testFunc() const
{
	return true;
}
