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

#ifndef TESTPOINTER_H
#define TESTPOINTER_H

#include <QtCore>
#include <QtTest>

class TestPointer : public QObject
{
	Q_OBJECT
	
	private slots:
		void single();
		
		void multi10();
		
		void multi100();
		
		void multi1000();
		
		void multi10000();
		
	private:
		void multi(int count);
};

#endif // TESTPOINTER_H
