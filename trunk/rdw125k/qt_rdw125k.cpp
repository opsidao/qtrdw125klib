/***************************************************************************
 *   Copyright (C) 2007 by Juan González Aguilera                          *
 *                         (kde_devel@opsiland.info)                       *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU Library General Public License as       *
 *   published by the Free Software Foundation; either version 2 of the    *
 *   License, or (at your option) any later version.                       *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU Library General Public     *
 *   License along with this program; if not, write to the                 *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#include "qt_rdw125k.h"
#include <QTimer>
#include <QDebug>
#include <QMetaType>
#include <QMutexLocker>

#undef DEBUG_READ_WRITE

Rdw125Control::Rdw125Control(QObject *parent)
	: QThread(parent),m_cardType(V0),m_data(QString()),m_lock(false)
{	
	qRegisterMetaType<Rdw125Control::OperationResult>("Rdw125Control::OpenPortResult");
	qRegisterMetaType<QPair<int,int> >("QPair<int,int>");
}


Rdw125Control::~Rdw125Control()
{
}

void Rdw125Control::close()
{
	port.flush();
	port.close();
}

void Rdw125Control::setCardType(CardType cardType)
{
	m_cardType = cardType;
}

Rdw125Control::CardType Rdw125Control::cardType()
{
	return m_cardType;
}

void Rdw125Control::setData(const QString & data)
{
	m_data = data.toUpper();
}

QString Rdw125Control::data()
{
	return m_data;
}

bool Rdw125Control::lockCard()
{
	return m_lock;
}

void Rdw125Control::setLockCard(bool lock)
{
	m_lock = lock;
}

void Rdw125Control::testNodeLink()
{
	QMutexLocker lock(&listMutex);
	requestQueue << TestLink;
	workingSemaphore.release(1);
}

void Rdw125Control::getFirmwareVersion()
{
	QMutexLocker lock(&listMutex);
	requestQueue << GetFirmwareVersion;
	workingSemaphore.release(1);
}

void Rdw125Control::readPublicModeA()
{
	QMutexLocker lock(&listMutex);
	requestQueue << Read125;
	workingSemaphore.release(1);
}

void Rdw125Control::writePublicModeA()
{
	QMutexLocker lock(&listMutex);
	requestQueue << Write125;
	workingSemaphore.release(1);
}

//Private implementation

QByteArray Rdw125Control::generateFrame(FrameType frameType, CardType cardType, QByteArray data, bool lockCard)
{
	//<STX><APID> <OPC><NA><ARG><CRC><ETX>
	QByteArray out;
	out.append((char)0x02);

	switch (frameType) {
		case TestLink:
			out.append("011000");			
			break;		
		case GetFirmwareVersion:
			out.append("011100");
			break;
		case PreRead125:
			out.append("010500");
			break;
		case Read125:
			out.append("011200");
			break;
		case Write125:
			out.append("011307");
			if(cardType == V0) {
				out.append("00");
			} else {
				out.append("01");
			}
			if (lockCard) {
				//TODO Change to FF when I trust the system
				qDebug() << "You were about to lock a card!!!, I ignored your request, you should edit the source code to enable card locking (qt_rdw125k.cpp:135)";
				out.append("00");
			} else {
				out.append("00");
			}
			out.append(data);
			break;
	}
	qulonglong suma = 0;
	for(int i = 1; i < out.size();i++)
	{
		suma+=out[i];
		suma%=255;
	}
	if (frameType == Write125)
		suma=(suma-3)%255;
	else
		suma=(suma-1)%255;
	
		
	QString sumaString = QString("%1").arg(suma,2,16,QLatin1Char('0'));
	
	out.append(sumaString.toUpper());
	out.append((char)0x03);	
	return out;
}

void Rdw125Control::_testNodelLink()
{
	if (!port.isOpen()) {
		emit testNodeLinkDone(NotOpen);
	}else {
		QByteArray frame = generateFrame(TestLink),exp="0190002A";
		port.flush();
		portWrite(frame);
		frame=portRead(exp.size()+2);
		port.flush();
		if (frame.indexOf(exp)==1) {
			emit testNodeLinkDone(Ok);
		} else {
			emit testNodeLinkDone(Failed);
		}
	}
}

void Rdw125Control::_getFirmwareVersion()
{
	
	QPair<int,int> out;
	if (port.isOpen()) {
		QByteArray frame = generateFrame(GetFirmwareVersion);
		port.flush();
		portWrite(frame);
		frame=portRead(14);
		port.flush();
		QByteArray answerPrefix("019102");
		answerPrefix.prepend((char)0x02);
		if (frame.startsWith(answerPrefix)) {
			QString num(2);
			num[0] = frame.at(7);
			num[1] = frame.at(8);
			out.first = num.toInt();
			num[0] = frame.at(9);
			num[1] = frame.at(10);
			out.second = num.toInt();
			emit getFirmwareVersionDone(true,out);
		} else {
			emit getFirmwareVersionDone(false,out);
		}
	} else {
		emit getFirmwareVersionDone(false,out);
	}	
}

void Rdw125Control::_readPublicModeA()
{
	if (!port.isOpen()) {
		emit readPublicModeDone(NotOpen);
	} else {
		QByteArray frame = generateFrame(Read125);
		port.flush();
		portWrite(frame);
		frame = portRead(7);
		if(frame.length() == 7) {
			if(frame[5]=='0' && frame[6]=='6') {
				frame.append(portRead(15));
				port.flush();
				if (frame.indexOf("019206") == 1) {
					QByteArray readData;
					for(int i = 7; i < 17; i++)
					{
						readData.append(frame.at(i));
					}
					qulonglong dec = readData.toULongLong(0,16);
					emit readPublicModeDone(Ok,readData,QString::number(dec));
				} else {
					emit readPublicModeDone(Failed);
				}
			} else {
				portRead(3);
				emit readPublicModeDone(Failed);
			}
		} else {
			emit readPublicModeDone(Failed);
		}
	}
}

void Rdw125Control::_writePublicModeA()
{
	if (!port.isOpen()) {
		emit writePublicModeDone(NotOpen);
	} else {
		Q_ASSERT(m_data.size() == 10);
		QByteArray frame = generateFrame(Write125,m_cardType,m_data.toAscii(),m_lock);
		port.flush();
		portWrite(frame);
		
		frame = portRead(12);
		if (frame.size()==12) {
			if (frame.at(7) == '0' && frame.at(8) == '0') {
				emit writePublicModeDone(Ok);
			} else {
				emit writePublicModeDone(Failed);
			}
		} else {
			emit writePublicModeDone(OperationError);
		}
	}
}

void Rdw125Control::run()
{
	FrameType request;
	bool ok;
	running = true;
	while(running)
	{
		workingSemaphore.acquire(1);
		ok = false;
		listMutex.lock();
		if (!requestQueue.isEmpty()) {
			request = requestQueue.dequeue();
			ok = true;
		}
		listMutex.unlock();
		if(ok) {
			switch(request) {
				case TestLink:
					_testNodelLink();
					break;
				case GetFirmwareVersion:
					_getFirmwareVersion();
					break;
				case Read125:
					_readPublicModeA();
					break;
				case Write125:
					_writePublicModeA();
					break;
					case PreRead125://Satisfy the compiler
						break;
			}
		}
	}
}

bool Rdw125Control::open()
{
	return port.open();
}

void Rdw125Control::setName(QString newName)
{
	port.setName(newName);
}

bool Rdw125Control::isOpen()
{
	return port.isOpen();
}

void Rdw125Control::end()
{
	running = false;
	workingSemaphore.release(1);
}

QByteArray Rdw125Control::portRead(int maxLength)
{
	QByteArray out = port.read(maxLength);
#ifdef DEBUG_READ_WRITE
	QByteArray tmp(out);
	qDebug() << "Read: " << tmp.replace((char)0x02,"<STX>").replace((char)0x03,"<ETX>");
#endif
	return out;
}

void Rdw125Control::portWrite(QByteArray data)
{
#ifdef DEBUG_READ_WRITE
	QByteArray tmp(data);
	qDebug() << "Writing: " << tmp.replace((char)0x02,"<STX>").replace((char)0x03,"<ETX>");
#endif
	port.write(data);
}







