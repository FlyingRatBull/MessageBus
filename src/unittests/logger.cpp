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


#include "logger.h"


Logger				*	Logger::s_inst	=	0;


Logger::Logger()
	:	m_logFile(0), m_logStream(0)
{
}


void Logger::setLogFile(const QString &filename)
{
	if(!s_inst)
		s_inst	=	new Logger();
	
	if(s_inst->m_logFile)
	{
		delete s_inst->m_logFile;
		s_inst->m_logFile	=	0;
	}
	
	if(s_inst->m_logStream)
	{
		delete s_inst->m_logStream;
		s_inst->m_logStream	=	0;
	}
	
	s_inst->m_logFile		=	new QFile(filename);
	
	if(!s_inst->m_logFile->open(QIODevice::WriteOnly | QIODevice::Append))
	{
		s_inst->m_logFile	=	0;
		
		qWarning("Could not open file \"%s\" for logging!", qPrintable(filename));
		
		return;
	}
	
	s_inst->m_logStream	=	new QTextStream(s_inst->m_logFile);
}


QString Logger::logFile()
{
	if(!s_inst)
		s_inst	=	new Logger();
	
	if(!s_inst->m_logFile)
		return QString();
	
	return s_inst->m_logFile->fileName();
}


void Logger::log(const QString &identifier, qreal value, const QString &annotation, const QString &annotationText)
{
	if(!s_inst)
		s_inst	=	new Logger();
	
	if(!s_inst->m_logStream)
		return;
	
	qint64	timestamp	=	QDateTime::currentMSecsSinceEpoch();
	
	QString	line(QString("%1.%2\t%3\t%4")
						.arg(timestamp/1000)															//	Datetime
						.arg(timestamp%1000)															//	Datetime
						.arg(QString(identifier).replace('\t', " "))							//	Identifier
						.arg(QString("%1").arg(value).replace(',', ".")));	//	Value
	
	if(!annotation.isEmpty())
	{
		line.append(";");
		line.append(annotation);
		
		if(!annotationText.isEmpty())
		{
			line.append(";");
			line.append(annotationText);
		}
	}
	
	line.append("\n");
	
	QWriteLocker		lock(&s_inst->m_logLock);
	(*s_inst->m_logStream)	<<	line;
	s_inst->m_logStream->flush();
}
