/***************************************************************************
 *   Copyright (C) 2007 by Opsidao,,,   *
 *   opsi@ka-tet   *
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


#include "ventana.h"
#include <QTimer>
#include <QInputDialog>
#include <QMessageBox>

Ventana::Ventana(QWidget* parent, Qt::WFlags fl)
: QMainWindow( parent, fl )
{
	setupUi(this);
	//TODO Enumerate ports more cleanly, with really available ports
	QStringList puertos;
#ifdef Q_WS_WIN
	puertos << "COM1" << "COM2"<< "COM3"<< "COM4";
#else
	puertos << "/dev/ttyS0" << "/dev/ttyS1"<< "/dev/ttyS2"<< "/dev/ttyS3";
#endif	
	puertos << "Otro";
	puerto->addItems(puertos);	
	
	QObject::connect(puerto,SIGNAL(activated( const QString& )),
			 this,SLOT(slotPuertoSeleccionado(const QString&)));
	QObject::connect(tipoDeTarjeta,SIGNAL(currentIndexChanged( const QString& )),
			 this,SLOT(slotUpdateCardType(const QString&)));
	
	QObject::connect(botonAbrirPuerto,SIGNAL(clicked()),
			 this,SLOT(slotAbrirPuerto()));
	
	QObject::connect(botonTestNodo,SIGNAL(clicked()),
			 this,SLOT(slotTestNodeLink()));
	QObject::connect(&control,SIGNAL(testNodeLinkDone(KRW125ctl::OperationResult)),
			 this,SLOT(slotTestNodeLinkDone(KRW125ctl::OperationResult)));
	
	QObject::connect(botonVersion,SIGNAL(clicked()),
			 this,SLOT(slotGetFirmwareVersion()));
	QObject::connect(&control,SIGNAL(getFirmwareVersionDone(bool, QPair< char, char >)),
			 this,SLOT(slotGetFirmwareVersionDone(bool, QPair< char, char >)));
	
	QObject::connect(botonLeerTarjeta,SIGNAL(clicked()),
			 this,SLOT(slotReadPublicMode()));
	QObject::connect(&control,SIGNAL(readPublicModeDone(KRW125ctl::OperationResult, const QString&, const QString&)),
			 this,SLOT(slotReadPublicModeDone(KRW125ctl::OperationResult, const QString&, const QString&)));
	
	QObject::connect(bloquearTarjeta,SIGNAL(toggled(bool)),this,SLOT(slotLockChanged(bool )));
}

Ventana::~Ventana()
{
}

/*$SPECIALIZATION$*/
void Ventana::slotPuertoSeleccionado(const QString& seleccionado)
{
	if (seleccionado == "Otro") {
		QString nPuerto = QInputDialog::getText(this,"Puerto personalizado","Introduzca el nombre del puerto a utilizar");
		if (!nPuerto.isEmpty()) {
			puerto->insertItem(puerto->count()-1,nPuerto);
			puerto->setCurrentIndex(puerto->count()-2);
		}
	}
}

void Ventana::slotUpdateCardType(const QString &value)
{
	statusBar()->showMessage(QString("Tipo de tarjeta cambiado a : %1").arg(value));
	if (value=="V0") {
		control.setCardType(KRW125ctl::V0);
	} else {
		control.setCardType(KRW125ctl::V1);
	}
}


void Ventana::slotAbrirPuerto()
{
	framePadre->setEnabled(false);
	if (control.isOpen()) {
		puerto->setEnabled(true);
		botonAbrirPuerto->setText("&Abrir puerto");
		control.close();
	} else {
		control.setName(puerto->currentText());
		QString msg;
		if (control.open()) {
			msg = "Se ha abierto el puerto correctamente";
			botonAbrirPuerto->setText("&Cerrar puerto");
			puerto->setEnabled(false);
			framePadre->setEnabled(true);
		} else {
			msg = "Imposible abrir puerto";
			puerto->setEnabled(true);
			framePadre->setEnabled(false);
		}
		statusBar()->showMessage(msg,0);
	}
}

void Ventana::slotTestNodeLink()
{
	progresoTest->setMaximum(0);
	framePadre->setEnabled(false);
	QTimer::singleShot(0,&control,SLOT(testNodelLink()));
}

void Ventana::slotTestNodeLinkDone(KRW125ctl::OperationResult result)
{
	framePadre->setEnabled(true);
	botonAbrirPuerto->setEnabled(true);
	progresoTest->setMaximum(1);	
	QString msg;
	switch(result)
	{
		case KRW125ctl::Ok:
			statusBar()->showMessage("Test del nodo correcto",0);
			msg = "Test correcto";
			break;
		case KRW125ctl::Failed:
		case KRW125ctl::NotOpen:
		case KRW125ctl::OperationError:
			statusBar()->showMessage("Ha fallado el test del nodo",0);
			msg = "Test incorrecto";
	}
	progresoTest->setFormat(msg);
}

void Ventana::slotGetFirmwareVersion()
{
	framePadre->setEnabled(false);
	QTimer::singleShot(0,&control,SLOT(getFirmwareVersion()));
}

void Ventana::slotGetFirmwareVersionDone(bool correct, QPair< char, char > version)
{
	framePadre->setEnabled(true);
	if (correct) {
		statusBar()->showMessage(QString::fromUtf8("Versión del firmware obtenida correctamente"),0);
		version0->setValue(version.first);
		version1->setValue(version.second);
	} else {
		statusBar()->showMessage(QString::fromUtf8("Ha fallado la obtención de la versión del firmware"),0);
		version0->setValue(-1);
		version1->setValue(-1);
	}
}


void Ventana::slotReadPublicMode()
{
	framePadre->setEnabled(false);
	QTimer::singleShot(0,&control,SLOT(readPublicModeA()));
}

void Ventana::slotReadPublicModeDone(KRW125ctl::OperationResult correct, const QString & hexData, const QString & decData)
{
	framePadre->setEnabled(true);
	switch(correct)
	{
		case KRW125ctl::Ok:
			statusBar()->showMessage("Tarjeta leida correctamente",0);
			leidoHex->setText(hexData);
			leidoDec->setText(decData);
			break;
		default:
			statusBar()->showMessage("No se pudo leer la tarjeta",0);
			leidoHex->setText("");
			leidoDec->setText("");
			break;
	}
}

void Ventana::slotLockChanged(bool lock)
{
	if (lock) {
		if(QMessageBox::question(this,"Habilitar bloqueo de tarjetas",QString::fromUtf8("<p>¿Esta seguro de que quiere activar el bloqueo de tarjetas?</p><p>Recuerde que las tarjetas bloqueadas no pueden ser sobreescritas</p>"),QMessageBox::Ok,QMessageBox::Cancel) == QMessageBox::Ok) {
			control.setLockCard(lock);		
		} else {
			bloquearTarjeta->setChecked(false);
		}
	} else {
		control.setLockCard(lock);
	}
}




