find_package(Qt5Core REQUIRED)
find_package(Qt5Network REQUIRED)
find_package(Qt5Test REQUIRED)

# Test: MessageBus
set(APPNAME "test_messagebus")

set(SOURCES
	testmessagebus.cpp
)

set(HEADERS
	testmessagebus.h
)

set(MOC_SRCS)
qt5_wrap_cpp(MOC_SRCS ${HEADERS})

add_executable(${APPNAME} ${SOURCES} ${MOC_SRCS})
target_link_libraries(${APPNAME} Qt5::Core Qt5::Network Qt5::Test ${CMAKE_PROJECT_NAME})
add_test(NAME ${APPNAME} COMMAND ${APPNAME} -xunitxml -o "${APPNAME}.xunit.xml")

# Test: MessageBus Peer
set(SOURCES
	testmessagebus_peer.cpp
)

set(HEADERS
	testmessagebus_peer.h
)

set(MOC_SRCS)
qt5_wrap_cpp(MOC_SRCS ${HEADERS})

add_executable(${APPNAME}_peer ${SOURCES} ${MOC_SRCS})
target_link_libraries(${APPNAME}_peer Qt5::Core Qt5::Network Qt5::Test ${CMAKE_PROJECT_NAME})