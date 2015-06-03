find_package(Qt5Core REQUIRED)
find_package(Qt5Network REQUIRED)

set(APPNAME "${CMAKE_PROJECT_NAME}")

set(SOURCES
	localserver.cpp
	localsocket.cpp
	localsocketprivate.cpp
	eventloop.cpp
	
 	messagebus.cpp
	messagebus_p.cpp
	
	tools.cpp
	variant.cpp
	)

set(HEADERS
	localserver.h
	localsocket.h
	localsocketprivate.h
	eventloop.h

 	messagebus.h
# 	messagebus_p.h
	
# 	tools.h
# 	tsdataqueue.h
# 	tsqueue.h
# 	variant.h
	)

set(INST_HEADERS
	LocalServer
	localserver.h
	LocalSocket
	localsocket.h

	MessageBus
	messagebus.h

	global.h
	tools.h
	tsdataqueue.h
	tsqueue.h
	Variant
	variant.h
	Pointer
	pointer.h
)

# OS specific implementations
if(${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
	set(SOURCES ${SOURCES}
		# Unix implementation
		implementations/localsocketprivate_unix.cpp
		)
	
	set(HEADERS ${HEADERS}
		# Unix implementation
		implementations/localsocketprivate_unix.h
		)
else()
	message(FATAL_ERROR "No local socket implementation for ${CMAKE_SYSTEM_NAME}!")
endif()

set(RESOURCES
	)

qt5_wrap_cpp(MOC_SOURCES ${HEADERS})
qt5_add_resources(RES_SOURCES ${RESOURCES})

# We are building the library
add_definitions(-DMSGBUS_DLL)

# Forward definitions
if(SIGNALS_FROM_THREADS)
  add_definitions(-DSIGNALS_FROM_THREADS)
endif()

# Forward definitions
if(USE_SELECT)
  add_definitions(-DUSE_SELECT)
endif()

# Create the actual executable
add_library("${APPNAME}" SHARED ${SOURCES} ${MOC_SOURCES} ${FORM_SOURCES} ${RES_SOURCES})
target_link_libraries("${APPNAME}"
    Qt5::Core
    Qt5::Network
	)

install(TARGETS "${APPNAME}"
	RUNTIME DESTINATION ${EXECUTABLE_INSTALL_PATH}
	LIBRARY DESTINATION ${LIBRARY_INSTALL_PATH}
	RUNTIME DESTINATION ${EXECUTABLE_INSTALL_PATH}
	)
# Install headers
install_headers(INST_HEADERS)

# Add unit tests
if(NOT NO_UNIT_TESTS)
	add_subdirectory(unittests)
endif()