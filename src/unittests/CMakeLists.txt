include_directories(${QT_INCLUDES})

# We are in a unit test
add_definitions(-DUNIT_TEST)

# Test: Queue
set(APPNAME "test_queue")

set(SOURCES
	testqueue.cpp
	)

set(HEADERS
# 	../tsqueue.h
	
	testqueue.h
	)

set(MOC_SRCS)
QT4_WRAP_CPP(MOC_SRCS ${HEADERS})

add_executable(${APPNAME} ${SOURCES} ${MOC_SRCS})
target_link_libraries(${APPNAME} ${QT_QTCORE_LIBRARY} ${QT_QTTEST_LIBRARY})
add_test(NAME ${APPNAME} COMMAND ${APPNAME} -xunitxml -o "${APPNAME}.xunit.xml")

# Test: Pointer
set(APPNAME "test_pointer")

set(SOURCES
	testpointer.cpp
	pointerclass.cpp
	)

set(HEADERS
# 	../pointer.h
	
	testpointer.h
# 	pointerclass.h
	)

set(MOC_SRCS)
QT4_WRAP_CPP(MOC_SRCS ${HEADERS})

add_executable(${APPNAME} ${SOURCES} ${MOC_SRCS})
target_link_libraries(${APPNAME} ${QT_QTCORE_LIBRARY} ${QT_QTTEST_LIBRARY})
add_test(NAME ${APPNAME} COMMAND ${APPNAME} -xunitxml -o "${APPNAME}.xunit.xml")


add_subdirectory(localsocket)
add_subdirectory(messagebus)