/*
 *  MessageBus - Inter process communication library
 *  Copyright (C) 2012  Oliver Becker <der.ole.becker@gmail.com>
 * 
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 * 
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 * 
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef GLOBAL_H
#define GLOBAL_H

// Generic helper definitions for shared library support
#if defined _WIN32 || defined __CYGWIN__
	#define MSGBUS_HELPER_DLL_IMPORT __declspec(dllimport)
	#define MSGBUS_HELPER_DLL_EXPORT __declspec(dllexport)
	#define MSGBUS_HELPER_DLL_LOCAL
	#define MSGBUS_HELPER_DLL_INTERNAL
#else
	#if __GNUC__ >= 4
		#define MSGBUS_HELPER_DLL_IMPORT __attribute__ ((visibility ("default")))
		#define MSGBUS_HELPER_DLL_EXPORT __attribute__ ((visibility ("default")))
		#define MSGBUS_HELPER_DLL_LOCAL  __attribute__ ((visibility ("hidden")))
		#define MSGBUS_HELPER_DLL_INTERNAL  __attribute__ ((visibility ("internal")))
	#else
		#define MSGBUS_HELPER_DLL_IMPORT
		#define MSGBUS_HELPER_DLL_EXPORT
		#define MSGBUS_HELPER_DLL_LOCAL
		#define MSGBUS_HELPER_DLL_INTERNAL
	#endif
#endif

// Now we use the generic helper definitions above to define MSGBUS_API and MSGBUS_LOCAL.
// MSGBUS_API is used for the public API symbols. It either DLL imports or DLL exports (or does nothing for static build)
// MSGBUS_LOCAL is used for non-api symbols.

#ifdef MSGBUS_DLL // defined if FOX is compiled as a DLL
	#ifdef MSGBUS_DLL_EXPORTS // defined if we are building the FOX DLL (instead of using it)
		#define MSGBUS_API MSGBUS_HELPER_DLL_EXPORT
	#else
		#define MSGBUS_API MSGBUS_HELPER_DLL_IMPORT
	#endif // MSGBUS_DLL_EXPORTS
	#define MSGBUS_LOCAL MSGBUS_HELPER_DLL_LOCAL
	#define MSGBUS_INTERNAL MSGBUS_HELPER_DLL_INTERNAL
#else // MSGBUS_DLL is not defined: this means FOX is a static lib.
	#define MSGBUS_API
	#define MSGBUS_LOCAL
	#define MSGBUS_INTERNAL
#endif // MSGBUS_DLL

#endif // GLOBAL_H
