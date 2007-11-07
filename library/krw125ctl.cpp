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

#include "krw125ctl.h"
#include <QTimer>

KRW125ctl::KRW125ctl(QObject *parent)
	: QextSerialPort("",parent),m_timeout(5000),m_cardType(V0),m_data(QString()),m_lock(false)
{
}


KRW125ctl::~KRW125ctl()
{
}

void KRW125ctl::close()
{
	flush();
	QextSerialPort::close();
}

void KRW125ctl::setCardType(CardType cardType)
{
	m_cardType = cardType;
}

KRW125ctl::CardType KRW125ctl::cardType()
{
	return m_cardType;
}

void KRW125ctl::setData(const QString & data)
{
	m_data = data;
}

QString KRW125ctl::data()
{
	return m_data;
}

bool KRW125ctl::lockCard()
{
	return m_lock;
}

void KRW125ctl::setLockCard(bool lock)
{
	m_lock = lock;
}

void KRW125ctl::testNodelLink()
{
	QTimer::singleShot(0,this,SLOT(_testNodelLink()));
}

void KRW125ctl::getFirmwareVersion()
{
	QTimer::singleShot(0,this,SLOT(_getFirmwareVersion()));
}

void KRW125ctl::readPublicModeA()
{
	QTimer::singleShot(0,this,SLOT(_readPublicModeA()));
}

void KRW125ctl::writePublicModeA()
{
	QTimer::singleShot(0,this,SLOT(_writePublicModeA()));
}

//Private implementation

QByteArray KRW125ctl::generateFrame(FrameType frameType, CardType cardType, QByteArray data, bool lockCard)
{
	//<STX><APID> <OPC><NA><ARG><CRC><ETX>
	QByteArray out;
	out.append((char)0x02); //STX
	out.append((char)0x01); //APID, nodo 1 del concentrador
	switch (frameType) {
		case TestLink:
			out.append((char)0x10);//OPC
			out.append((char)0x00);//NA
			break;
		case GetFirmwareVersion:
			out.append((char)0x11);//OPC
			out.append((char)0x00);//NA
			break;
		case Read125:
			out.append((char)0x12);//OPC
			out.append((char)0x00);//NA
			break;
		case Write125:
			Q_ASSERT(data.size()==5);
			out.append((char)0x13);//OPC
			out.append((char)0x07);//NA
			out.append((char)cardType);
			if (lockCard) {
				out.append((char)0xFF);
			} else {
				out.append((char)0x00);
			}
			out.append(data);
			break;
	}
	//Calculate CRC
	char crc = 0;
	for (int i = 1; i < out.size();i++) {
		crc = (crc+out.at(i))%255;
	}
	out.append(crc);
	out.append((char)0x03);//ETX
	return out;
}

bool KRW125ctl::checkFrameCRC(QByteArray frame)
{
	if (frame.size() < 6) {
		return false;
	}
	int crc = 0;
	for (int i = 1; i < frame.size()-2;i++) {
		crc = (crc+frame.at(i))%255;
	}
	return crc == frame.at(frame.size()-1);
}

void KRW125ctl::setTimeout(int timeout)
{
	m_timeout = timeout;
}

int KRW125ctl::timeout()
{
	return m_timeout;
}

void KRW125ctl::_testNodelLink()
{
	if (!isOpen()) {
		emit testNodeLinkDone(NotOpen);
	}else {
		QByteArray frame = generateFrame(TestLink);
		write(frame);
		waitForReadyRead(m_timeout);
		frame = readAll();
		if (frame.size()==6 && checkFrameCRC(frame)) {
			emit testNodeLinkDone(Ok);
		} else {
			emit testNodeLinkDone(Failed);
		}
	}
}

void KRW125ctl::_getFirmwareVersion()
{
	QPair<char,char> out;
	if (isOpen()) {
		QByteArray frame = generateFrame(GetFirmwareVersion);
		write(frame);
		waitForReadyRead(m_timeout);
		frame = read(8);
		if (frame.size() == 8 && checkFrameCRC(frame)) {
			out.first = frame.at(4);
			out.second = frame.at(5);
			emit getFirmwareVersionDone(true,out);
		} else {
			emit getFirmwareVersionDone(false,out);
		}
	} else {
		emit getFirmwareVersionDone(false,out);
	}	
}

void KRW125ctl::_readPublicModeA()
{
	if (!isOpen()) {
		emit readPublicModeDone(NotOpen);
	} else {
		QByteArray frame = generateFrame(Read125);
		write(frame);
		waitForReadyRead(m_timeout);
		frame = read(12);
		if (frame.size() >= 6 && checkFrameCRC(frame)) {
			if (frame.at(3) == 0x00) {
				Q_ASSERT(frame.size() == 6);
				emit readPublicModeDone(Failed);
			} else {
				Q_ASSERT(frame.size() == 12);
				QByteArray data;
				for (int i = 4; i < frame.size()-3; i++) { //TODO We are ignoring the CRC inside the read data
					data.append(frame.at(i));
				}
				QString hex = data.toHex();
				QString dec = QString::number(data.toDouble());
				emit readPublicModeDone(Ok,hex,dec);
			}
		} else {
			emit readPublicModeDone(OperationError);
		}
	}
}

void KRW125ctl::_writePublicModeA()
{
	if (!isOpen()) {
		emit writePublicModeDone(NotOpen);
	} else {
		Q_ASSERT(m_data.size() == 10);
		QByteArray frame = generateFrame(Write125,m_cardType,m_data.toAscii(),m_lock);
		write(frame);
		waitForReadyRead(m_timeout);
		frame = read(7);
		if (frame.size()==7 && checkFrameCRC(frame)) {
			if (frame.at(4) == 0x00	) {
				emit writePublicModeDone(Ok);
			} else {
				emit writePublicModeDone(Failed);
			}
		} else {
			emit writePublicModeDone(OperationError);
		}
	}
}







