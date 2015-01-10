# Test: MessageBus
set(APPNAME "test_messagebus")

set(SOURCES
	testmessagebus.cpp
)

set(HEADERS
	testmessagebus.h
)

set(MOC_SRCS)
QT4_WRAP_CPP(MOC_SRCS ${HEADERS})

add_executable(${APPNAME} ${SOURCES} ${MOC_SRCS})
target_link_libraries(${APPNAME} ${QT_QTCORE_LIBRARY} ${QT_QTNETWORK_LIBRARY} ${QT_QTTEST_LIBRARY} ${CMAKE_PROJECT_NAME})
add_test(NAME ${APPNAME} COMMAND ${APPNAME} -xml -o "${APPNAME}.xunit.xml")

# Test: MessageBus Peer
set(SOURCES
	testmessagebus_peer.cpp
)

set(HEADERS
	testmessagebus_peer.h
)

set(MOC_SRCS)
QT4_WRAP_CPP(MOC_SRCS ${HEADERS})

add_executable(${APPNAME}_peer ${SOURCES} ${MOC_SRCS})
target_link_libraries(${APPNAME}_peer ${QT_QTCORE_LIBRARY} ${QT_QTNETWORK_LIBRARY} ${QT_QTTEST_LIBRARY} ${CMAKE_PROJECT_NAME})