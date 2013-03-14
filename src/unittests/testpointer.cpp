#include "testpointer.h"

#include "../pointer.h"

#include "pointerclass.h"

void TestPointer::single()
{
	QAtomicInt							counter;
	Pointer<PointerClass>		ptr(new PointerClass(counter));
	
	QVERIFY(ptr != 0);
	QVERIFY(counter.fetchAndAddOrdered(0) == 1);
	QVERIFY(ptr->testFunc());
	
	ptr	=	0;
	
	QVERIFY(ptr == 0);
	QVERIFY(counter.fetchAndAddOrdered(0) == 0);
}


void TestPointer::multi10()
{
	multi(10);
}


void TestPointer::multi100()
{
	multi(100);
}


void TestPointer::multi1000()
{
	multi(1000);
}


void TestPointer::multi10000()
{
	multi(10000);
}


void TestPointer::multi(int count)
{
	QAtomicInt											counter;
	PointerClass									*	ptrClass	=	new PointerClass(counter);
	QList<Pointer<PointerClass> >		ptrList;
	
	QVERIFY(counter.fetchAndAddOrdered(0) == 1);
	
	// Create pointer
	for(int i = 0; i < count; i++)
	{
		ptrList.append(Pointer<PointerClass>(ptrClass));
		
		QVERIFY(ptrList.last() != 0);
		QVERIFY(counter.fetchAndAddOrdered(0) == 1);
		QVERIFY(ptrList.last()->testFunc());
	}
	
	// Remove pointer (all but one)
	for(int i = count - 1; i > 0; i--)
	{
		ptrList.removeLast();
		
		QVERIFY(ptrList.last() != 0);
		QVERIFY(counter.fetchAndAddOrdered(0) == 1);
		QVERIFY(ptrList.last()->testFunc());
	}
	
	Pointer<PointerClass>	ptr(ptrList.takeLast());
	ptr	=	0;
	
	QVERIFY(ptr == 0);
	QVERIFY(counter.fetchAndAddOrdered(0) == 0);
}


QTEST_MAIN(TestPointer)
