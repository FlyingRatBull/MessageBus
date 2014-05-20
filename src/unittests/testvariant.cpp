#include "testvariant.h"

#include <QVariantMap>

#include "../variant.h"

void TestVariant::testQMap()
{
  QVariantMap   map;
  
  map.insert("string", "teststring");
  map.insert("integer", 123);
  
  Variant var(map);
  QVariantMap map2(var.toQMap());
  
  QCOMPARE(map, map2);
}


void TestVariant::testQList()
{
  QVariantList   list;
  
  list.append("teststring");
  list.append(123);
  
  Variant var(list);
  QVariantList list2(var.toQList());
  
  QCOMPARE(list, list2);
}


QTEST_MAIN(TestVariant)
