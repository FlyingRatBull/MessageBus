include_directories(${QT_INCLUDES})

# We are in a unit test
add_definitions(-DUNIT_TEST)

# Test: Queue
set(APPNAME "test_queue")

set(SOURCES
	
	testqueue.cpp
	)

set(HEADERS
	../tsqueue.h
	
	testqueue.h
	)

set(MOC_SRCS)
QT4_WRAP_CPP(MOC_SRCS ${HEADERS})

add_executable(${APPNAME} ${SOURCES} ${MOC_SRCS})
target_link_libraries(${APPNAME} ${QT_QTCORE_LIBRARY} ${QT_QTTEST_LIBRARY})


# Test: LocalSocket
set(APPNAME "test_localsocket")

set(SOURCES
	testlocalsocket.cpp
	testlocalsocket_peer.cpp
	)

set(HEADERS
	testlocalsocket.h
	testlocalsocket_peer.h
	)

set(MOC_SRCS)
QT4_WRAP_CPP(MOC_SRCS ${HEADERS})

add_executable(${APPNAME} ${SOURCES} ${MOC_SRCS})
target_link_libraries(${APPNAME} ${QT_QTCORE_LIBRARY} ${QT_QTNETWORK_LIBRARY} ${QT_QTTEST_LIBRARY} ${CMAKE_PROJECT_NAME})
# 
# set(SOURCES
# 	testlocalsocket_external_peer.cpp
# 	testlocalsocket_external_peer_object.cpp
# 	)
# 
# set(HEADERS
# 	testlocalsocket_external_peer_object.h
# 	)
# 
# set(MOC_SRCS)
# QT4_WRAP_CPP(MOC_SRCS ${HEADERS})
# 
# add_executable("${APPNAME}_external_peer" ${SOURCES} ${MOC_SRCS})
# target_link_libraries("${APPNAME}_external_peer" ${QT_QTCORE_LIBRARY} ${QT_QTNETWORK_LIBRARY} ${QT_QTTEST_LIBRARY})


# Test: MessageBus
set(APPNAME "test_messagebus")

set(SOURCES
	testmessagebus.cpp
	testmessagebus_peer.cpp
	)

set(HEADERS
	testmessagebus.h
	testmessagebus_peer.h
	)

set(MOC_SRCS)
QT4_WRAP_CPP(MOC_SRCS ${HEADERS})

add_executable(${APPNAME} ${SOURCES} ${MOC_SRCS})
target_link_libraries(${APPNAME} ${QT_QTCORE_LIBRARY} ${QT_QTNETWORK_LIBRARY} ${QT_QTTEST_LIBRARY}  ${CMAKE_PROJECT_NAME})
