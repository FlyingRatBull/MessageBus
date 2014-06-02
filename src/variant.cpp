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

#include "variant.h"

#include <QVariant>
#include <QString>
#include <unistd.h>

Variant::Variant()
: m_type(None), m_optId(0), m_autoCloseAndDup(false)
{
}


Variant::Variant(Variant::Type type)
: m_type(type), m_optId(0), m_autoCloseAndDup(false)
{

}


Variant::Variant(qint8 num)
: m_type(Int8), m_optId(0), m_autoCloseAndDup(false)
{
	setValue(num);
}


Variant::Variant(quint8 num)
: m_type(UInt8), m_optId(0), m_autoCloseAndDup(false)
{
	setValue(num);
}


Variant::Variant(qint16 num)
: m_type(Int16), m_optId(0), m_autoCloseAndDup(false)
{
	setValue(num);
}


Variant::Variant(quint16 num)
: m_type(UInt16), m_optId(0), m_autoCloseAndDup(false)
{
	setValue(num);
}


Variant::Variant(qint32 num)
: m_type(Int32), m_optId(0), m_autoCloseAndDup(false)
{
	setValue(num);
}


Variant::Variant(quint32 num)
: m_type(UInt32), m_optId(0), m_autoCloseAndDup(false)
{
	setValue(num);
}


Variant::Variant(qint64 num)
: m_type(Int64), m_optId(0), m_autoCloseAndDup(false)
{
	setValue(num);
}


Variant::Variant(quint64 num)
: m_type(UInt64), m_optId(0), m_autoCloseAndDup(false)
{
	setValue(num);
}


Variant::Variant(const QString& string)
: m_type(String), m_optId(0), m_autoCloseAndDup(false)
{
	setValue(string);
}


Variant::Variant(const QVariantMap& map)
: m_type(Map), m_optId(0), m_autoCloseAndDup(false)
{
  setValue(map);
}


Variant::Variant(const QVariantList& list)
: m_type(List), m_optId(0), m_autoCloseAndDup(false)
{
  setValue(list);
}


Variant::Variant(const QList< Variant >& list)
: m_type(List), m_optId(0), m_autoCloseAndDup(false)
{
  setValue(list);
}


Variant::Variant(bool boolean)
: m_type(Bool), m_optId(0), m_autoCloseAndDup(false)
{
	setValue(boolean);
}


Variant::Variant(const QByteArray& data)
: m_type(ByteArray), m_data(data), m_optId(0), m_autoCloseAndDup(false)
{
}


Variant::Variant(const QByteArray &data, Variant::Type type, quint32 optId)
: m_type(type), m_data(data), m_optId(optId), m_autoCloseAndDup(false)
{
}


Variant::Variant(const Variant& other)
{
  (*this) = other;
}


Variant::Variant(const QVariant& other)
: m_type(None), m_optId(0)
{
	(*this)	=	other;
}


Variant::~Variant()
{
  if(m_type == SocketDescriptor && m_autoCloseAndDup) {
    ::close(int(toInt64()));
  }
}


Variant& Variant::operator = (const Variant& other)
{
	m_data	=	other.m_data;
	m_type	=	other.m_type;
	m_optId	=	other.m_optId;
  m_autoCloseAndDup = other.m_autoCloseAndDup;
  
  if(m_type == SocketDescriptor && m_autoCloseAndDup) {
    bool    ok;
    qint64  old = getIntNumber(sizeof(quint64), &ok);
    setValue(qint64(dup(int(old))));
  }
  
	return *this;
}


Variant& Variant::operator = (const QVariant& other)
{
  m_autoCloseAndDup = false;
  
	switch(other.type())
	{
		case QVariant::Int:
		{
			switch(sizeof(int))
			{
				case sizeof(qint8):
				{
					m_type	=	Int8;
					setValue((qint8)other.toInt());
				}break;
				
				case sizeof(qint16):
				{
					m_type	=	Int16;
					setValue((qint16)other.toInt());
				}break;
				
				case sizeof(qint32):
				{
					m_type	=	Int32;
					setValue((qint32)other.toInt());
				}break;
				
				case sizeof(qint64):
				{
					m_type	=	Int64;
					setValue((qint64)other.toInt());
				}break;
			}
		}break;
		
		case QVariant::LongLong:
		{
			switch(sizeof(qlonglong))
			{
				case sizeof(qint8):
				{
					m_type	=	Int8;
					setValue((qint8)other.toLongLong());
				}break;
				
				case sizeof(qint16):
				{
					m_type	=	Int16;
					setValue((qint16)other.toLongLong());
				}break;
				
				case sizeof(qint32):
				{
					m_type	=	Int32;
					setValue((qint32)other.toLongLong());
				}break;
				
				case sizeof(qint64):
				{
					m_type	=	Int64;
					setValue((qint64)other.toLongLong());
				}break;
			}
		}break;
		
		case QVariant::UInt:
		{
			switch(sizeof(uint))
			{
				case sizeof(quint8):
				{
					m_type	=	UInt8;
					setValue((quint8)other.toUInt());
				}break;
				
				case sizeof(quint16):
				{
					m_type	=	UInt16;
					setValue((quint16)other.toUInt());
				}break;
				
				case sizeof(quint32):
				{
					m_type	=	UInt32;
					setValue((quint32)other.toUInt());
				}break;
				
				case sizeof(quint64):
				{
					m_type	=	UInt64;
					setValue((quint64)other.toUInt());
				}break;
			}
		}break;
		
		case QVariant::ULongLong:
		{
			switch(sizeof(qulonglong))
			{
				case sizeof(quint8):
				{
					m_type	=	UInt8;
					setValue((quint8)other.toULongLong());
				}break;
				
				case sizeof(quint16):
				{
					m_type	=	UInt16;
					setValue((quint16)other.toULongLong());
				}break;
				
				case sizeof(quint32):
				{
					m_type	=	UInt32;
					setValue((quint32)other.toULongLong());
				}break;
				
				case sizeof(quint64):
				{
					m_type	=	UInt64;
					setValue((quint64)other.toULongLong());
				}break;
			}
		}break;
		
		case QVariant::String:
		{
			m_type	=	String;
			setValue(other.toString());
    }break;
    
    case QVariant::ByteArray:
    {
      m_type  = ByteArray;
      setValue(other.toByteArray());
    }break;
    
    case QVariant::Map:
    {
      m_type  = Map;
      setValue(other.toMap());
    }break;
    
    case QVariant::List:
    {
      m_type  = List;
      setValue(other.toList());
    }break;
		
		case QVariant::Bool:
		{
			m_type	=	Bool;
			setValue(other.toBool());
		}break;

		default:
		{
			m_type	=	Variant::None;
		}break;
	}
	
	return *this;
}


Variant& Variant::operator = (const QByteArray& other)
{
  m_autoCloseAndDup = false;
	m_type	=	ByteArray;
	setValue(other);
	return *this;
}


Variant& Variant::operator = (const QString& other)
{
  m_autoCloseAndDup = false;
  m_type  = String;
  setValue(other);
  return *this;
}


Variant& Variant::operator = (const QVariantMap& other)
{
  m_autoCloseAndDup = false;
  m_type  = Map;
  setValue(other);
  return *this;
}


Variant& Variant::operator = (const QVariantList& other)
{
  m_autoCloseAndDup = false;
  m_type  = List;
  setValue(other);
  return *this;
}


Variant& Variant::operator=(const QList< Variant >& other)
{
  m_autoCloseAndDup = false;
  m_type  = List;
  setValue(other);
  return *this;
}


Variant& Variant::operator = (bool boolean)
{
  m_autoCloseAndDup = false;
	m_type	=	Bool;
	setValue(boolean);
	return *this;
}


Variant& Variant::operator = (quint8 num)
{
  m_autoCloseAndDup = false;
	m_type	=	UInt8;
	setValue(num);
	return *this;
}


Variant& Variant::operator = (qint8 num)
{
  m_autoCloseAndDup = false;
	m_type	=	Int8;
	setValue(num);
	return *this;
}


Variant& Variant::operator = (quint16 num)
{
  m_autoCloseAndDup = false;
	m_type	=	UInt16;
	setValue(num);
	return *this;
}


Variant& Variant::operator = (qint16 num)
{
  m_autoCloseAndDup = false;
	m_type	=	Int16;
	setValue(num);
	return *this;
}


Variant& Variant::operator = (quint32 num)
{
  m_autoCloseAndDup = false;
	m_type	=	UInt32;
	setValue(num);
	return *this;
}


Variant& Variant::operator = (qint32 num)
{
  m_autoCloseAndDup = false;
	m_type	=	Int32;
	setValue(num);
	return *this;
}


Variant& Variant::operator = (quint64 num)
{
  m_autoCloseAndDup = false;
	m_type	=	UInt64;
	setValue(num);
	return *this;
}


Variant& Variant::operator = (qint64 num)
{
  m_autoCloseAndDup = false;
	m_type	=	Int64;
	setValue(num);
	return *this;
}


void Variant::setOptionalId(quint32 optId)
{
	m_optId	=	optId;
}


quint32 Variant::optionalId() const
{
	return m_optId;
}


void Variant::setValue(qint8 num)
{
	m_data	=	QByteArray((char*)&num, sizeof(qint8));
}


void Variant::setValue(quint8 num)
{
	m_data	=	QByteArray((char*)&num, sizeof(quint8));
}


void Variant::setValue(qint16 num)
{
	m_data	=	QByteArray((char*)&num, sizeof(qint16));
}


void Variant::setValue(quint16 num)
{
	m_data	=	QByteArray((char*)&num, sizeof(quint16));
}


void Variant::setValue(qint32 num)
{
	m_data	=	QByteArray((char*)&num, sizeof(qint32));
}


void Variant::setValue(quint32 num)
{
	m_data	=	QByteArray((char*)&num, sizeof(quint32));
}


void Variant::setValue(qint64 num)
{
	m_data	=	QByteArray((char*)&num, sizeof(qint64));
}


void Variant::setValue(quint64 num)
{
	m_data	=	QByteArray((char*)&num, sizeof(quint64));
}


void Variant::setValue(const QByteArray& data)
{
	m_data	=	data;
}


void Variant::setValue(const QString& value)
{
  m_data  = value.toUtf8();
}


void Variant::setValue(const QVariantMap& value)
{
  QByteArray  data;
  QByteArray  key;
  data.reserve(sizeof(quint64) + value.count() * (sizeof(quint32) * 2 + 32));
  
  // Format:
  // [item count][key size][UTF8 key][value size][value]...
  
  // item count
  quint64 itemCount = value.count();
  data.resize(data.size() + sizeof(itemCount));
  *((quint64*)(&(data.data()[data.size() - sizeof(itemCount)]))) = itemCount;
  
  QMapIterator<QString, QVariant>   it(value);
  while(it.hasNext()) {
    it.next();
    
    key = it.key().toUtf8();
    
    // key size
    quint32 size = key.size();
    data.resize(data.size() + sizeof(size));
    *((quint32*)(&(data.data()[data.size() - sizeof(size)]))) = size;
    
    // key
    data.append(key);
    
    Variant     val(Variant(it.value()));
    QByteArray  value(val.toByteArray());
    
    // value type
    quint16 type = quint16(val.type());
    data.resize(data.size() + sizeof(type));
    *((quint16*)(&(data.data()[data.size() - sizeof(type)]))) = type;
    
    // value size
    size = value.size();
    data.resize(data.size() + sizeof(size));
    *((quint32*)(&(data.data()[data.size() - sizeof(size)]))) = size;
    
    // value
    data.append(value);
  }
  
  m_data  = data;
}


void Variant::setValue(const QVariantList& value)
{
  QByteArray  data;
  QByteArray  key;
  data.reserve(sizeof(quint64) + value.count() * (sizeof(quint32) * 2 + 32));
  
  // Format:
  // [item count][value size][value]...
  
  // item count
  quint64 itemCount = value.count();
  data.resize(data.size() + sizeof(itemCount));
  *((quint64*)(&(data.data()[data.size() - sizeof(itemCount)]))) = itemCount;
  
  QListIterator<QVariant>   it(value);
  while(it.hasNext()) {
    Variant     val(Variant(it.next()));
    QByteArray  value(val.toByteArray());
    
    // value type
    quint16 type = quint16(val.type());
    data.resize(data.size() + sizeof(type));
    *((quint16*)(&(data.data()[data.size() - sizeof(type)]))) = type;
    
    // value size
    quint32 size = value.size();
    data.resize(data.size() + sizeof(size));
    *((quint32*)(&(data.data()[data.size() - sizeof(size)]))) = size;    
    
    // value
    data.append(value);
  }
  
  m_data  = data;
}


void Variant::setValue(const QList< Variant >& value)
{
  QByteArray  data;
  QByteArray  key;
  data.reserve(sizeof(quint64) + value.count() * (sizeof(quint32) * 2 + 32));
  
  // Format:
  // [item count][value size][value]...
  
  // item count
  quint64 itemCount = value.count();
  data.resize(data.size() + sizeof(itemCount));
  *((quint64*)(&(data.data()[data.size() - sizeof(itemCount)]))) = itemCount;
  
  QListIterator<Variant>   it(value);
  while(it.hasNext()) {
    Variant     val(it.next());
    QByteArray  value(val.toByteArray());
    
    // value type
    quint16 type = quint16(val.type());
    data.resize(data.size() + sizeof(type));
    *((quint16*)(&(data.data()[data.size() - sizeof(type)]))) = type;
    
    // value size
    quint32 size = value.size();
    data.resize(data.size() + sizeof(size));
    *((quint32*)(&(data.data()[data.size() - sizeof(size)]))) = size;
    
    // value
    data.append(value);
  }
  
  m_data  = data;
}


void Variant::setValue(bool boolean)
{
	setValue((char)boolean);
}


quint8 Variant::toUInt8(bool * ok) const
{
	///@todo Check if number is greater than max value
	return (quint8)toUInt64(ok);
}


qint8 Variant::toInt8(bool * ok) const
{
	///@todo Check if number is greater than max value
	return (qint8)toInt64(ok);
}


quint16 Variant::toUInt16(bool * ok) const
{
	///@todo Check if number is greater than max value
	return (quint16)toUInt64(ok);
}


qint16 Variant::toInt16(bool * ok) const
{
	///@todo Check if number is greater than max value
	return (qint16)toInt64(ok);
}


quint32 Variant::toUInt32(bool * ok) const
{
	///@todo Check if number is greater than max value
	return (quint32)toUInt64(ok);
}


qint32 Variant::toInt32(bool * ok) const
{
	///@todo Check if number is greater than max value
	return (qint32)toInt64(ok);
}


quint64 Variant::toUInt64(bool * ok) const
{
	if(ok)
		(*ok)	=	false;
	
	switch(m_type)
	{
		case Bool:
		case UInt8:
			return (quint64)getUIntNumber(sizeof(quint8), ok);break;
			
		case UInt16:
			return (quint64)getUIntNumber(sizeof(quint16), ok);break;
			
		case UInt32:
			return (quint64)getUIntNumber(sizeof(quint32), ok);break;
			
		case UInt64:
			return (quint64)getUIntNumber(sizeof(quint64), ok);break;
			
		case Int8:
			return (quint64)getIntNumber(sizeof(qint8), ok);break;
			
		case Int16:
			return (quint64)getIntNumber(sizeof(qint16), ok);break;
			
		case Int32:
			return (quint64)getIntNumber(sizeof(qint32), ok);break;
			
		case Int64:
			return (quint64)getIntNumber(sizeof(qint64), ok);break;
			
		case SocketDescriptor:
			return (quint64)getIntNumber(sizeof(qint64), ok);break;
			
		case ByteArray:
			return (quint64)getIntNumber(sizeof(qint64), ok);break;
		
		case String:
			return toString(ok).toULongLong(ok);break;
      
    case List: {
      QVariantList  list(toQList(ok));
      
      if(ok && !*ok)
        return 0;
      
      if(!list.isEmpty())
        return list.at(0).toULongLong(ok);
      
      return 0;
    }break;

    default:
      return 0;break;
	}
}


qint64 Variant::toInt64(bool * ok) const
{
	if(ok)
		(*ok)	=	false;
	
	switch(m_type)
	{
		case Bool:
		case UInt8:
			return (qint64)getUIntNumber(sizeof(quint8), ok);break;
			
		case UInt16:
			return (qint64)getUIntNumber(sizeof(quint16), ok);break;
			
		case UInt32:
			return (qint64)getUIntNumber(sizeof(quint32), ok);break;
			
		case UInt64:
			return (qint64)getUIntNumber(sizeof(quint64), ok);break;
			
		case Int8:
			return (qint64)getIntNumber(sizeof(qint8), ok);break;
			
		case Int16:
			return (qint64)getIntNumber(sizeof(qint16), ok);break;
			
		case Int32:
			return (qint64)getIntNumber(sizeof(qint32), ok);break;
			
		case Int64:
			return (qint64)getIntNumber(sizeof(qint64), ok);break;
			
		case SocketDescriptor:
			return (qint64)getIntNumber(sizeof(qint64), ok);break;
			
		case ByteArray:
			return (qint64)getIntNumber(sizeof(qint64), ok);break;
		
		case String:
			return toString(ok).toLongLong(ok);break;
      
      
      
    case List: {
      QVariantList  list(toQList(ok));
      
      if(ok && !*ok)
        return 0;
      
      if(!list.isEmpty())
        return list.at(0).toLongLong(ok);
      
      return 0;
    }break;

    default:
      return 0; break;
	}
}


int Variant::toSocketDescriptor(bool *ok) const
{
	if(ok)
		(*ok)	=	false;
	
	if(m_type	==	SocketDescriptor)
	{
		qint64	ret	=	getIntNumber(sizeof(qint64), ok);
    
    if(m_autoCloseAndDup && (!ok || *ok)) {
      int  newFd = dup(int(ret));
      return newFd;
    }
		
		return (int)ret;
	}
	
	return 0;
}


QByteArray Variant::toByteArray(bool * ok) const
{
	if(ok)
		(*ok)	=	true;
	
	return m_data;
}


QString Variant::toString(bool * ok) const
{
	if(ok)
		(*ok)	=	true;
	
	///@todo Complete switch
	switch(m_type)
	{
		case ByteArray:
		{
			return m_data.toHex();
		};
		
		case String:
		{
			return QString::fromUtf8(m_data);
		}break;
    
    case Map: {
      QVariantMap map(toQMap());
      QString     ret("{");
        
      QMapIterator<QString, QVariant>   it(map);
      
      while(it.hasNext()) {
        it.next();
        
        ret += "\"" + it.key() + "\":" + it.value().toString();
        
        if(it.hasNext())
          ret += ",";
      }
      
      ret += "}";
      
      return ret;
    }break;
    
    case List: {
      QVariantList  list(toQList());
      QString       ret("[");
        
      QListIterator<QVariant>   it(list);
        
      while(it.hasNext()) {
        ret += it.next().toString();
        
        if(it.hasNext())
          ret += ",";
      }
      
      ret += "]";
      
      return ret;
    }break;

		case Int8:
		{
			return QString::number(toInt8());
		}break;

		case Int16:
		{
			return QString::number(toInt16());
		}break;

		case Int32:
		{
			return QString::number(toInt32());
		}break;

		case Int64:
		{
			return QString::number(toInt64());
		}break;

		case UInt8:
		{
			return QString::number(toUInt8());
		}break;

		case UInt16:
		{
			return QString::number(toUInt16());
		}break;

		case UInt32:
		{
			return QString::number(toUInt32());
		}break;

		case UInt64:
		{
			return QString::number(toUInt64());
		}break;
		
		case SocketDescriptor:
		{
			return QString::number(toSocketDescriptor());
		}break;
		
		case Bool:
		{
			if(toBool())
				return "true";
			else
				return "false";
		}break;
		
		default:
		{
			if(ok)
				(*ok)	=	false;
			
			return QString();
		}break;
	}
}


QVariantMap Variant::toQMap(bool* ok) const
{
  if(ok)
    (*ok) = true;
  
  ///@todo Complete switch
  switch(m_type) {
    case Map: {
      QVariantMap ret;
      int         idx = 0;
      
      // Read map
      
      // Format:
      // [item count][key size][UTF8 key][value size][value]...
      
      // item count
      quint64 itemCount = *((quint64*)&(m_data.data()[idx]));
      idx += sizeof(itemCount);
      
      for(quint64 i = 0; i < itemCount; i++) {
        // Key size
        quint32 size = *((quint32*)&(m_data.data()[idx]));
        idx += sizeof(size);
        
        // Key
        QString key(QString::fromUtf8(m_data.mid(idx, size)));
        idx += size;
        
        // Value type
        quint16 type = *((quint16*)&(m_data.data()[idx]));
        idx += sizeof(type);
        
        // Value size
        size = *((quint32*)&(m_data.data()[idx]));
        idx += sizeof(size);
        
        // Value
        Variant value(m_data.mid(idx, size), (Variant::Type)type);
        idx += size;
        
        ret.insert(key, value.toQVariant());
      }
      
      return ret;
    }break;
    
    case List: {
      QVariantList  list(toQList(ok));
      QVariantMap   ret;
      
      if(ok && !*ok)
        return ret;
      
      QListIterator<QVariant>   it(list);
      int                       idx = 0;
      
      while(it.hasNext()) {
        ret.insert(QString::number(idx), it.next());
        
        ++idx;
      }
      
      return ret;
    }break;
    
    default:
    {
      if(ok)
        (*ok) = false;
      
      return QVariantMap();
    }break;
  }
}


QVariantList Variant::toQList(bool* ok) const
{
  if(ok)
    (*ok) = true;
  
  ///@todo Complete switch
  switch(m_type) {
    case Map: {
      return toQMap().values();
    }break;
    
    case List: {
      QVariantList  ret;
      int           idx = 0;
      
      // Read list
      
      // Format:
      // [item count][value size][value]...
      
      // item count
      quint64 itemCount = *((quint64*)&(m_data.data()[idx]));
      idx += sizeof(itemCount);
      
      for(int i = 0; i < itemCount; i++) {
        // Value type
        quint16 type = *((quint16*)&(m_data.data()[idx]));
        idx += sizeof(type);
        
        // Value size
        quint32 size = *((quint32*)&(m_data.data()[idx]));
        idx += sizeof(size);
        
        // Value
        Variant value(m_data.mid(idx, size), (Variant::Type)type);
        idx += size;
        
        ret.append(value.toQVariant());
      }
      
      return ret;
    }break;
    
    default:
    {
      // Return list with first value being variant value
      QVariantList  ret;
      ret.append(toQVariant());
      
      return ret;
    }break;
  }
}


QList<Variant> Variant::toList(bool* ok) const
{
  if(ok)
    (*ok) = true;
  
  ///@todo Complete switch
  switch(m_type) {
    case List: {
      QList<Variant>  ret;
      int           idx = 0;
      
      // Read list
      
      // Format:
      // [item count][value size][value]...
      
      // item count
      quint64 itemCount = *((quint64*)&(m_data.data()[idx]));
      idx += sizeof(itemCount);
      
      for(int i = 0; i < itemCount; i++) {
        // Value type
        quint16 type = *((quint16*)&(m_data.data()[idx]));
        idx += sizeof(type);
        
        // Value size
        quint32 size = *((quint32*)&(m_data.data()[idx]));
        idx += sizeof(size);
        
        // Value
        Variant value(m_data.mid(idx, size), (Variant::Type)type);
        idx += size;
        
        ret.append(value);
      }
      
      return ret;
    }break;
    
    default:
    {
      // Return list with first value being variant value
      QList<Variant>  ret;
      ret.append(*this);
      
      return ret;
    }break;
  }
}


QVariant Variant::toQVariant(bool* ok) const
{
  switch(m_type) {
    case Variant::Bool:
      return QVariant(toBool(ok));break;
      
    case Variant::Int8:
      return QVariant(toInt8(ok));break;
      
    case Variant::Int16:
      return QVariant(toInt16(ok));break;
      
    case Variant::Int32:
      return QVariant(toInt32(ok));break;
      
    case Variant::Int64:
      return QVariant(toInt64(ok));break;
      
    case Variant::UInt8:
      return QVariant(toUInt8(ok));break;
      
    case Variant::UInt16:
      return QVariant(toUInt16(ok));break;
      
    case Variant::UInt32:
      return QVariant(toUInt32(ok));break;
      
    case Variant::UInt64:
      return QVariant(toUInt64(ok));break;
      
    case Variant::ByteArray:
      return QVariant(toByteArray(ok));break;
      
    case Variant::String:
      return QVariant(toString(ok));break;
      
    case Variant::Map:
      return QVariant(toQMap(ok));break;
      
    case Variant::List:
      return QVariant(toQList(ok));break;
      
    
    default: {
      if(ok)
        *ok = false;
      
      return QVariant();
    }break;
  }
}


bool Variant::toBool(bool* ok) const
{
	switch(m_type)
	{
		case UInt8:
		case UInt16:
		case UInt32:
		case UInt64:
		case Int8:
		case Int16:
		case Int32:
		case Int64:
		case Bool:
			return ((qint8)toInt64(ok) ? true : false);
			
		case SocketDescriptor:
			return (toInt64(ok) > 0 ? true : false);
			
		case String:
			return toString().trimmed().toLower() == "true";
			
		default:
		{
			if(ok)
				*ok	=	false;
			
			return false;
		}break;
	}
}


Variant::Type Variant::type() const
{
  return m_type;
}


Variant Variant::fromUInt8(quint8 num)
{
	return Variant(num);
}


Variant Variant::fromInt8(qint8 num)
{
	return Variant(num);
}


Variant Variant::fromUInt16(quint16 num)
{
	return Variant(num);
}


Variant Variant::fromInt16(qint16 num)
{
	return Variant(num);
}


Variant Variant::fromUInt32(quint32 num)
{
	return Variant(num);
}


Variant Variant::fromInt32(qint32 num)
{
	return Variant(num);
}


Variant Variant::fromUInt64(quint64 num)
{
	return Variant(num);
}


Variant Variant::fromInt64(qint64 num)
{
	return Variant(num);
}


Variant Variant::fromSocketDescriptor(int socketDescriptor, bool autoCloseAndDup)
{
	Variant	ret(SocketDescriptor);
	ret.setValue((qint64)socketDescriptor);
  ret.m_autoCloseAndDup = autoCloseAndDup;

	return ret;
}


Variant Variant::fromString(const QString& string)
{
	///@todo Extended detection of types
	
	///@todo Test for quint8 (only numbers)
	///@todo if - then qint8
	///@todo if greater than 255 -> q(u)int16 -> q(u)int32 -> q(u)int64
	
	Variant	var(string);
	return var;
}


Variant Variant::fromByteArray(const QByteArray &data)
{
  return Variant(data);
}


Variant Variant::fromByteArray(const char* data, int size)
{
  return Variant(QByteArray(data, size));
}


Variant Variant::fromMap(const QVariantMap& map)
{
  return Variant(map);
}


Variant Variant::fromList(const QVariantList& list)
{
  return Variant(list);
}



Variant Variant::fromList(const QList< Variant >& list)
{
  return Variant(list);
}


bool Variant::operator == (const Variant& other) const
{
  return (m_type == other.m_type && m_data == other.m_data && m_optId == other.m_optId);
}


bool Variant::operator != (const Variant& other) const
{
  return !(*this == other);
}


bool Variant::isValid() const
{
	return (m_type != None);
}


int Variant::size() const
{
	return m_data.size();
}


quint64 Variant::getUIntNumber(quint8 size, bool * ok) const
{
	if(ok)
		(*ok)	=	true;
	
	if(size > m_data.size())
	{
		if(ok)
			(*ok)	=	false;
		
		return 0;
	}
	
	switch(size)
	{
		case sizeof(quint8):
			return (quint64)*((quint8*)m_data.data());break;
			
		case sizeof(quint16):
			return (quint64)*((quint16*)m_data.data());break;
			
		case sizeof(quint32):
			return (quint64)*((quint32*)m_data.data());break;
			
		case sizeof(quint64):
			return (quint64)*((quint64*)m_data.data());break;
	}
	
	if(ok)
		(*ok)	=	false;
	
	return 0;
}


qint64 Variant::getIntNumber(quint8 size, bool * ok) const
{
	if(ok)
		(*ok)	=	true;
	
	if(size > m_data.size())
	{
		if(ok)
			(*ok)	=	false;
		
		return 0;
	}
	
	switch(size)
	{
		case sizeof(qint8):
			return (qint64)*((qint8*)m_data.data());break;
			
		case sizeof(qint16):
			return (qint64)*((qint16*)m_data.data());break;
			
		case sizeof(qint32):
			return (qint64)*((qint32*)m_data.data());break;
			
		case sizeof(qint64):
			return (qint64)*((qint64*)m_data.data());break;
	}
	
	if(ok)
		(*ok)	=	false;
	
	return 0;
}


void Variant::writeIntelligentNumber(QByteArray& target, quint64 num, quint8 shift)
{
  ///@todo Use 2 bits (right shifted by \a shift) to mark size of following integer (8/16/32/64)
}


quint64 Variant::readIntelligentNumber(const QByteArray& source, int idx, int& readBytes)
{

}


int __initVariant()
{
	qRegisterMetaType<Variant>("Variant");
	return 1;
}

Q_CONSTRUCTOR_FUNCTION(__initVariant)