include_directories(${QT_INCLUDES})
include_directories(${CMAKE_CURRENT_BINARY_DIR})

set(APPNAME "${CMAKE_PROJECT_NAME}")

set(SOURCES
	localserver.cpp
	localsocket.cpp
	localsocketprivate_unix.cpp
	
	messagebus.cpp
	messagebus_p.cpp
	messagebusinterface.cpp
	
	tools.cpp
	variant.cpp
	)

set(HEADERS
	localserver.h
	localsocket.h
	localsocketprivate_unix.h
	
	messagebus.h
# 	messagebus_p.h
	messagebusinterface.h
	messagebusinterface_p.h
	
# 	tools.h
# 	tsdataqueue.h
# 	tsqueue.h
# 	variant.h
	)

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
	
add_custom_command(TARGET "${APPNAME}"
	POST_BUILD
	COMMAND "${PROJECT_SOURCE_DIR}/raise_version.sh"
	WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}"
	)
	
install(TARGETS "${APPNAME}"
	RUNTIME DESTINATION ${EXECUTABLE_INSTALL_PATH}
	LIBRARY DESTINATION ${LIBRARY_INSTALL_PATH}
	RUNTIME DESTINATION ${EXECUTABLE_INSTALL_PATH}
	)


# Add unit tests
add_subdirectory(unittests)
