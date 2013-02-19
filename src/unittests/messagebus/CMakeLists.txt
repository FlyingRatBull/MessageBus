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
add_test(NAME ${APPNAME} COMMAND ${APPNAME})
