include_directories(${QT_INCLUDES})
include_directories(${CMAKE_CURRENT_BINARY_DIR})

set(APPNAME "${CMAKE_PROJECT_NAME}")

set(SOURCES
	localserver.cpp
	localsocket.cpp
	localsocketprivate.cpp
	
	messagebus.cpp
	messagebus_p.cpp
	messagebusinterface.cpp
	
	tools.cpp
	variant.cpp
	)

set(HEADERS
	localserver.h
	localsocket.h
	localsocketprivate.h
	localsocketprivate_thread.h

	messagebus.h
# 	messagebus_p.h
	messagebusinterface.h
	messagebusinterface_p.h
	
# 	tools.h
# 	tsdataqueue.h
# 	tsqueue.h
# 	variant.h
	)

set(INST_HEADERS
	localserver.h
	localsocket.h

	messagebus.h
	messagebusinterface.h

	tools.h
	tsdataqueue.h
	tsqueue.h
	variant.h
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

set(FORMS
	)

set(RESOURCES
	)

qt4_wrap_cpp(MOC_SOURCES ${HEADERS})
qt4_wrap_ui(FORM_SOURCES ${FORMS})
qt4_add_resources(RES_SOURCES ${RESOURCES})

# We are building the library
add_definitions(-DMSGBUS_DLL)

# Create the actual executable
add_library("${APPNAME}" SHARED ${SOURCES} ${MOC_SOURCES} ${FORM_SOURCES} ${RES_SOURCES})
target_link_libraries("${APPNAME}" ${QT_QTCORE_LIBRARY}
	#${QT_QTGUI_LIBRARY}
	#${QT_QTSQL_LIBRARY}
# 	${QT_QTNETWORK_LIBRARY}
	)

install(TARGETS "${APPNAME}"
	RUNTIME DESTINATION ${EXECUTABLE_INSTALL_PATH}
	LIBRARY DESTINATION ${LIBRARY_INSTALL_PATH}
	RUNTIME DESTINATION ${EXECUTABLE_INSTALL_PATH}
	)
# Install headers
install_headers(INST_HEADERS)

# Add unit tests
add_subdirectory(unittests)