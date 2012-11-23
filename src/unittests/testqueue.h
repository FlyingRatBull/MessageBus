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

#ifndef TESTQUEUE_H
#define TESTQUEUE_H

#include <QtCore>
#include <QtTest>


class TestQueue : public QObject
{
	Q_OBJECT
	
	private slots:
		void ts_queueItems();
		
		void ts_dequeueItems();
		
		void ts_randomAction();
		
		void ts_queueData();
		
		void ts_dequeueData();
		
		void benchmark();
		
		void qt_benchmark();
		
		void ts_benchmark();
		
		// Data
		void ts_queueItems_data();
		
		void ts_dequeueItems_data();
		
		void ts_randomAction_data();
		
		void ts_queueData_data();
		
		void ts_dequeueData_data();
		
		void benchmark_data();
		
		void qt_benchmark_data();
		
		void ts_benchmark_data();
};

#endif // TESTQUEUE_H
