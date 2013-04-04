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


void TestPointer::multiRef10()
{
	multiRef(10);
}


void TestPointer::multiRef100()
{
	multiRef(100);
}


void TestPointer::multiRef1000()
{
	multiRef(1000);
}


void TestPointer::multiRef10000()
{
	multiRef(10000);
}


void TestPointer::multiVal10()
{
	multiVal(10);
}


void TestPointer::multiVal100()
{
	multiVal(100);
}


void TestPointer::multiVal1000()
{
	multiVal(1000);
}


void TestPointer::multiVal10000()
{
	multiVal(10000);
}


void TestPointer::multiValMultiRef10()
{
	multiValMultiRef(10);
}


void TestPointer::multiValMultiRef100()
{
	multiValMultiRef(100);
}


void TestPointer::multiValMultiRef1000()
{
	multiValMultiRef(1000);
}


void TestPointer::multiValMultiRef10000()
{
	multiValMultiRef(10000);
}


void TestPointer::multiRef(int count)
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


void TestPointer::multiVal(int count)
{
	QAtomicInt											counter;
	QList<Pointer<PointerClass> >		ptrList;
	
	QVERIFY(counter == 0);
	
	// Create pointer
	for(int i = 0; i < count; i++)
	{
		ptrList.append(Pointer<PointerClass>(new PointerClass(counter)));
		
		QVERIFY(ptrList.last() != 0);
		QVERIFY(counter == i + 1);
		QVERIFY(ptrList.last()->testFunc());
	}
	
	// Remove pointer (all but one)
	for(int i = count - 1; i > 0; i--)
	{
		ptrList.removeLast();
		
		QVERIFY(ptrList.last() != 0);
		QVERIFY(counter == i);
		QVERIFY(ptrList.last()->testFunc());
	}
	
	Pointer<PointerClass>	ptr(ptrList.takeLast());
	ptr	=	0;
	
	QVERIFY(ptr == 0);
	QVERIFY(counter == 0);
}


void TestPointer::multiValMultiRef(int count)
{
	QAtomicInt											counter;
	QList<Pointer<PointerClass> >		ptrList;
	QList<Pointer<PointerClass> >		ptrCopyList;
	
	QVERIFY(counter == 0);
	
	// Create pointer
	for(int i = 0; i < count; i++)
	{
		ptrList.append(Pointer<PointerClass>(new PointerClass(counter)));
		
		QVERIFY(ptrList.last() != 0);
		QVERIFY(counter == i + 1);
		QVERIFY(ptrList.last()->testFunc());
	}
	
	// Create duplicate pointer
	foreach(const Pointer<PointerClass>& ptr, ptrList)
	{
		ptrCopyList.append(ptr);
		
		QVERIFY(ptrCopyList.last() != 0);
		QVERIFY(counter == count);
		QVERIFY(ptrCopyList.last()->testFunc());
	}

	// Remove pointer
	for(int i = count - 1; i > 0; i--)
	{
		ptrList.removeLast();
		
		QVERIFY(ptrList.last() != 0);
		QVERIFY(counter == count);
		QVERIFY(ptrList.last()->testFunc());
	}
	
	{
		Pointer<PointerClass>	ptr(ptrList.takeLast());
		ptr	=	0;
		
		QVERIFY(ptr == 0);
		QVERIFY(counter == count);
	}

	// Remove copy pointer
	for(int i = count - 1; i > 0; i--)
	{
		ptrCopyList.removeLast();
		
		QVERIFY(ptrCopyList.last() != 0);
		QVERIFY(counter == i);
		QVERIFY(ptrCopyList.last()->testFunc());
	}
	
	{
		Pointer<PointerClass>	ptr(ptrCopyList.takeLast());
		ptr	=	0;
		
		QVERIFY(ptr == 0);
		QVERIFY(counter == 0);
	}
}


QTEST_MAIN(TestPointer)
