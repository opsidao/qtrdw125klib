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
#ifndef KRW125CTL_H
#define KRW125CTL_H

#include <QObject>
#include <QPair>
#include <QThread>
#include <QMutex>
#include <QQueue>
#include <QWaitCondition>

#include "qextserialport.h"

class KRW125ctl : public QThread
{
	Q_OBJECT
	Q_ENUMS(CardType)
	Q_ENUMS(OperationResult)
	Q_PROPERTY(QString data READ data WRITE setData)
	Q_PROPERTY(bool lockCard READ lockCard WRITE setLockCard)

	public:
		enum OperationResult {Ok,Failed,NotOpen,OperationError};
		
		enum CardType {V0=0x00, V1=0x01};

		KRW125ctl(QObject *parent = 0);
		~KRW125ctl();

		void close();

		void setCardType(CardType cardType);
		CardType cardType();

		void setData(const QString &data);
		QString data();

		void setLockCard(bool lock);
		bool lockCard();

		bool open();
		bool isOpen();
		void setName(QString newName);
		void run();
		void end();

	public slots:
		void testNodeLink();
		void getFirmwareVersion();
		void readPublicModeA();
		void writePublicModeA();

	protected slots:
		void _testNodelLink();
		void _getFirmwareVersion();
		void _readPublicModeA();
		void _writePublicModeA();

	signals:
		void testNodeLinkDone(int result);
		void getFirmwareVersionDone(bool correct, QPair<int,int> version );
		void readPublicModeDone(int correct, const QString &hexData = QString(), const QString &decData =QString());
		void writePublicModeDone(int correct);
		void nodeTimeout();

	protected:
		enum FrameType {TestLink,GetFirmwareVersion,PreRead125,Read125,Write125};
		
		QByteArray generateFrame(FrameType frameType, CardType cardType=V0, QByteArray data = QByteArray(),  bool lockCard = false);
		
		//bool checkFrameCRC(QByteArray frame);
		
		QQueue<FrameType> requestQueue;
		QMutex listMutex;
		QextSerialPort port;
		CardType m_cardType;
		QString m_data;
		bool m_lock;
		bool running;
};

#endif
