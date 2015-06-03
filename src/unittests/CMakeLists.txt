find_package(Qt5Core REQUIRED)
find_package(Qt5Test REQUIRED)

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
qt5_wrap_cpp(MOC_SRCS ${HEADERS})

add_executable(${APPNAME} ${SOURCES} ${MOC_SRCS})
target_link_libraries(${APPNAME} Qt5::Core Qt5::Test)
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
qt5_wrap_cpp(MOC_SRCS ${HEADERS})

add_executable(${APPNAME} ${SOURCES} ${MOC_SRCS})
target_link_libraries(${APPNAME} Qt5::Core Qt5::Test)
add_test(NAME ${APPNAME} COMMAND ${APPNAME} -xunitxml -o "${APPNAME}.xunit.xml")

# Test: Variant
set(APPNAME "test_variant")

set(SOURCES
testvariant.cpp
../variant.cpp
)

set(HEADERS
#   ../variant.h

testvariant.h
)

set(MOC_SRCS)
qt5_wrap_cpp(MOC_SRCS ${HEADERS})

add_executable(${APPNAME} ${SOURCES} ${MOC_SRCS})
target_link_libraries(${APPNAME} Qt5::Core Qt5::Test)
add_test(NAME ${APPNAME} COMMAND ${APPNAME} -xunitxml -o "${APPNAME}.xunit.xml")


# add_subdirectory(localsocket)
add_subdirectory(messagebus)