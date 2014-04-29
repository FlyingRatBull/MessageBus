/*
 *  MessageBus - Inter process communication library
 *  Copyright (C) 2013  Oliver Becker <der.ole.becker@gmail.com>
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

#ifndef TESTMESSAGEBUS_PEER_1_H
#define TESTMESSAGEBUS_PEER_1_H

#include <QObject>

#include "../../variant.h"
#include "../../messagebus.h"

class TestMessageBus_Peer :  QObject
{
	Q_OBJECT
	
	public:
		TestMessageBus_Peer();
		
		~TestMessageBus_Peer();
		
	public slots:
		void voidCall(MessageBus * src, const Variant& arg1 = Variant(), const Variant& arg2 = Variant(), const Variant& arg3 = Variant(), const Variant& arg4 = Variant());
		
	private slots:
		void onDisconnected();
		
	private:
		MessageBus		*	m_bus;

};

#endif // TESTMESSAGEBUS_PEER_1_H
