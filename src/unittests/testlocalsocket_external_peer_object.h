// Copyright 2012 Oliver Becker <der.ole.becker@gmail.com>
// 
// This file is part of the MessageBus project,
// 
// MessageBus is free software: you can redistribute it and/or modify it
// under the terms of the GNU General Public License as published by the Free Software Foundation,
// either version 3 of the License, or (at your option) any later version.
// MessageBus is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
// without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
// See the GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License along with MessageBus.
// If not, see http://www.gnu.org/licenses/.

#ifndef PEEROBJECT_H
#define PEEROBJECT_H

#include <QtCore>


class PeerObject : public QObject
{
	Q_OBJECT
	
	public:
		MessageBusPeer();
		
		virtual ~MessageBusPeer();
		
	public slots:

	private:
		void init();
		
	private:
		MessageBus		*	m_messageBus;
};

#endif // PEEROBJECT_H
