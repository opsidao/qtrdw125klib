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
#include <QDebug>
#include <QMetaType>
#include <QMutexLocker>

KRW125ctl::KRW125ctl(QObject *parent)
	: QThread(parent),m_cardType(V0),m_data(QString()),m_lock(false)
{	
	qRegisterMetaType<KRW125ctl::OperationResult>("KRW125ctl::OpenPortResult");
	qRegisterMetaType<QPair<int,int> >("QPair<int,int>");
}


KRW125ctl::~KRW125ctl()
{
}

void KRW125ctl::close()
{
	port.flush();
	port.close();
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

void KRW125ctl::testNodeLink()
{
	QMutexLocker lock(&listMutex);
	requestQueue << TestLink;
}

void KRW125ctl::getFirmwareVersion()
{
	QMutexLocker lock(&listMutex);
	requestQueue << GetFirmwareVersion;
}

void KRW125ctl::readPublicModeA()
{
	QMutexLocker lock(&listMutex);
	requestQueue << Read125;
}

void KRW125ctl::writePublicModeA()
{
	QMutexLocker lock(&listMutex);
	requestQueue << Write125;
}

//Private implementation

QByteArray KRW125ctl::generateFrame(FrameType frameType, CardType cardType, QByteArray data, bool lockCard)
{
	//<STX><APID> <OPC><NA><ARG><CRC><ETX>
	QByteArray out;
	out.append('0'); //APID, nodo 1 del concentrador
	out.append('1');

	switch (frameType) {
		case TestLink:
			out.append('0');//OPC
			out.append('0');
			out.append('0');//NA
			out.append('0');
			out.append('2');//CRC
			out.append('1');
			break;
		case TestLinkAnswer:
			out.append('8');//OPC
			out.append('0');
			out.append('0');//NA
			out.append('0');
			out.append('2');//CRC
			out.append('9');
			break;
		case GetFirmwareVersion:
			out.append('1');//OPC
			out.append('1');
			out.append('0');//NA
			out.append('0');
			out.append('2');//CRC
			out.append('3');
			break;
		case Read125:
			out.append('0');//OPC
			out.append('1');
			out.append('0');//NA
			out.append('0');
			out.append('2');//CRC
			out.append('2');
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
	out.append((char)0x03);
	return out;
}

bool KRW125ctl::checkFrameCRC(QByteArray frame)
{
	qDebug() << "Checking answer's CRC";
	if (frame.size() < 6) {
		return false;
	}
	int crc = 0;
	for (int i = 1; i < frame.size()-2;i++) {
		crc = (crc+frame.at(i))%255;
	}
	QString crcSt = QString::number(crc-12);
	qDebug() <<"Checked crc " << crcSt;
	return frame.indexOf(crcSt)==(frame.size()-3);

}

// void KRW125ctl::setTimeout(int timeout)
// {
// 	m_timeout = timeout;
// }
// 
// int KRW125ctl::timeout()
// {
// 	return m_timeout;
// }

void KRW125ctl::_testNodelLink()
{
	if (!port.isOpen()) {
		emit testNodeLinkDone(NotOpen);
	}else {
		QByteArray frame = generateFrame(TestLink),exp=generateFrame(TestLinkAnswer);
		port.write(frame);
		port.flush();
		frame=port.read(exp.size());
		if (frame == exp) {
			emit testNodeLinkDone(Ok);
		} else {
			emit testNodeLinkDone(Failed);
		}
	}
}

void KRW125ctl::_getFirmwareVersion()
{
	
	QPair<int,int> out;
	if (port.isOpen()) {
		QByteArray frame = generateFrame(GetFirmwareVersion);
		port.write(frame);
		frame=port.read(14);
		QByteArray answerPrefix("019102");
		answerPrefix.prepend((char)0x02);
		if (frame.startsWith(answerPrefix)) {
			qDebug()<<"Answer prefix OK :";
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

void KRW125ctl::_readPublicModeA()
{
	if (!port.isOpen()) {
		emit readPublicModeDone(NotOpen);
	} else {
		QByteArray frame = generateFrame(Read125);
		port.write(frame);
		
		frame = port.read(12);
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
	if (!port.isOpen()) {
		emit writePublicModeDone(NotOpen);
	} else {
		Q_ASSERT(m_data.size() == 10);
		QByteArray frame = generateFrame(Write125,m_cardType,m_data.toAscii(),m_lock);
		port.write(frame);
		
		frame = port.read(7);
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

void KRW125ctl::readyReadSlot()
{
	qDebug() << "Read ready";
}

void KRW125ctl::run()
{
	while(1)
	{
		FrameType request;
		bool ok = false;
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
				case TestLinkAnswer://Satisfy the compiler
					break;
			}
		} else {
			QThread::msleep(150);
		}
	}
}

bool KRW125ctl::open()
{
	return port.open();
}

void KRW125ctl::setName(QString newName)
{
	port.setName(newName);
}

bool KRW125ctl::isOpen()
{
	return port.isOpen();
}






