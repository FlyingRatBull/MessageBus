include_directories(${QT_INCLUDES})
include_directories(${CMAKE_CURRENT_BINARY_DIR})

set(APPNAME "${CMAKE_PROJECT_NAME}")

set(SOURCES
	localserver.cpp
	localsocket.cpp
	
	messagebus.cpp
	messagebus_p.cpp
	messagebusinterface.cpp
	
	tools.cpp
	variant.cpp
	)

set(HEADERS
	localserver.h
	localsocket.h
	# Don't include here, it's just an template for new implementations
# 	localsocketprivate.h

	messagebus.h
# 	messagebus_p.h
	messagebusinterface.h
	messagebusinterface_p.h
	
# 	tools.h
# 	tsdataqueue.h
# 	tsqueue.h
# 	variant.h
	)

# OS specific implementations
if(${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
	set(SOURCES ${SOURCES}
		# Unix implementation
		localsocketprivate_unix.cpp
		localsocketprivate_unix_worker.cpp
		)
	
	set(HEADERS ${HEADERS}
		# Unix implementation
		localsocketprivate_unix.h
		localsocketprivate_unix_worker.h
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


# Add unit tests
add_subdirectory(unittests)
