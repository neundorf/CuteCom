/*
 * Copyright (c) 2015 Meinhard Ritscher <cyc1ingsir@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 * For more information on the GPL, please go to:
 * http://www.gnu.org/copyleft/gpl.html
 */

#include "statusbar.h"
#include <QString>
#include <QtSerialPort/QtSerialPort>
#include <QSerialPortInfo>

StatusBar::StatusBar(QWidget *parent)
    : QWidget(parent)
{
    setupUi(this);
}

void StatusBar::sessionChanged(const Settings::Session &session)
{
    QString parity;
    switch (session.parity) {
    case QSerialPort::NoParity:
        parity = QStringLiteral("N");
        break;
    case QSerialPort::MarkParity:
        parity = QStringLiteral("Mark");
        break;
    case QSerialPort::SpaceParity:
        parity = QStringLiteral("Space");
        break;
    case QSerialPort::EvenParity:
        parity = QStringLiteral("Even");
        break;
    case QSerialPort::OddParity:
        parity = QStringLiteral("Odd");
        break;
    default:
        parity = QStringLiteral("?");
        break;
    }
    QString stopBits;
    switch (session.stopBits) {
    case QSerialPort::OneStop:
        stopBits = QString::number(1);
        break;
    case QSerialPort::OneAndHalfStop:
        stopBits = QString::number(1.5);
        break;
    case QSerialPort::TwoStop:
        stopBits = QString::number(2);
        break;
    default:
        break;
    }

    QString connectionParameter
        = QString("%1 @ %2-%3-%4").arg(session.baudRate).arg(session.dataBits).arg(parity).arg(stopBits);
    m_lb_portparams->setText(connectionParameter);

    if (!session.device.isEmpty())
        m_lb_deviceName->setText(session.device);
    QWidget::setToolTip(QStringLiteral(""));
}

void StatusBar::setDeviceInfo(const QSerialPort *port)
{
    QSerialPortInfo info = QSerialPortInfo(*port);
    if (info.isValid()) {
        QString deviceInfo = QString("%1 %2 @%3").arg(info.manufacturer()).arg(info.description()).arg(info.portName());
        m_lb_deviceName->setText(deviceInfo);
    }
}

void StatusBar::setToolTip(const QSerialPort *port)
{

    QSerialPortInfo info = QSerialPortInfo(*port);
    if (info.isValid()) {
        QString deviceInfo = QString("%1 %2\n%3:%4 # %5")
                                 .arg(info.manufacturer())
                                 .arg(info.description())
                                 .arg(info.vendorIdentifier())
                                 .arg(info.productIdentifier())
                                 .arg(info.serialNumber());
        QWidget::setToolTip(deviceInfo);
    } else {
        QWidget::setToolTip(tr("Not a valid device"));
    }
}
