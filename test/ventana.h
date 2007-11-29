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

#ifndef VENTANA_H
#define VENTANA_H

#include <QMainWindow>
#include "ui_ventana.h"
#include "qt_rdw125k.h"

class Ventana : public QMainWindow, private Ui::MainWindow
{
	Q_OBJECT

	public:
		Ventana(QWidget* parent = 0, Qt::WFlags fl = 0 );
		~Ventana();
		/*$PUBLIC_FUNCTIONS$*/

	public slots:
		/*$PUBLIC_SLOTS$*/

	protected:
		Rdw125Control control;

	protected slots:
		void slotPuertoSeleccionado(const QString& seleccionado);
		void slotUpdateCardType(const QString&);
		void slotAbrirPuerto();
		void slotTestNodeLink();
		void slotTestNodeLinkDone(int result);
		void slotGetFirmwareVersion();
		void slotGetFirmwareVersionDone(bool correct, QPair<int,int> version);
		void slotReadPublicMode();
		void slotReadPublicModeDone(int correct, const QString &hexData, const QString &decData);
		void slotLockChanged(bool lock);
		void validateData2Write(const QString&);
		void slotWritePublicMode();
		void slotWritePublicModeDone(int correct);
};

#endif

