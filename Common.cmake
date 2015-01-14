#	Enable testing (incl. CDash)
enable_testing()
include(CTest)

include(version.cmake)

if(EXISTS "${PROJECT_SOURCE_DIR}/Definitions.cmake")
	include("${PROJECT_SOURCE_DIR}/Definitions.cmake")
endif()

# Set package config file
# set(CPACK_PROJECT_CONFIG_FILE )
if(NOT CMAKE_CURRENT_LIST_DIR)
	get_filename_component(CMAKE_CURRENT_LIST_DIR ${CMAKE_CURRENT_LIST_FILE}
		PATH)
endif()

# set(CPACK_PROJECT_CONFIG_FILE ${CMAKE_CURRENT_LIST_DIR}/Package.cmake)

# Set build dir
if(${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
	
	set(RESOURCES_INSTALL_PATH "share/${CMAKE_PROJECT_NAME}/")
	set(EXECUTABLE_INSTALL_PATH "bin/")
	set(LIBRARY_INSTALL_PATH "lib/")
	set(INCLUDE_INSTALL_PATH "include/${CMAKE_PROJECT_NAME}/")
	
elseif(${CMAKE_SYSTEM_NAME} STREQUAL "Windows")
	
	set(RESOURCES_INSTALL_PATH "res/")
	set(EXECUTABLE_INSTALL_PATH "bin/")
	set(LIBRARY_INSTALL_PATH "bin/")
	set(INCLUDE_INSTALL_PATH "include/")
	
else()
	
	message(FATAL_ERROR "Not prepared for ${CMAKE_SYSTEM_NAME}!")
	
endif()
#add_definitions(-DRES_INSTALL_PATH="${RESOURCES_INSTALL_PATH}" -DEXE_INSTALL_PATH="${EXECUTABLE_INSTALL_PATH}" -DLIB_INSTALL_PATH="${LIBRARY_INSTALL_PATH}" )

MACRO(install_headers HEADER_LIST)
	FOREACH(HEADER ${${HEADER_LIST}})
		STRING(REGEX MATCH "(.*)[/\\]" DIR ${HEADER})
		INSTALL(FILES ${HEADER} DESTINATION ${INCLUDE_INSTALL_PATH}/${DIR})
	ENDFOREACH(HEADER)
ENDMACRO(install_headers)


# Include version information
# include(${PROJECT_SOURCE_DIR}/Version.cmake)

if(NOT CMAKE_BUILD_TYPE)
	set(CMAKE_BUILD_TYPE "Debug")
endif()

if(${CMAKE_BUILD_TYPE} STREQUAL "Debug")
	add_definitions(-DDEBUG)
endif()

# Debugging
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g -DDEBUG")
