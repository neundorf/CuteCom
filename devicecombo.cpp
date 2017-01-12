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

#include "devicecombo.h"

#include <QSerialPortInfo>
#include "qdebug.h"

void DeviceCombo::showPopup()
{
    refill();
    QComboBox::showPopup();
}

void DeviceCombo::refill()
{
    QString selection = currentText();
    clear();
    int numberDevices = 0;
    for (auto info : QSerialPortInfo::availablePorts()) {
        addItem(info.systemLocation());
        if (info.isValid()) {
            QString deviceInfo = QString("%1 %2\n%3:%4 "
#if QT_VERSION < QT_VERSION_CHECK(5, 3, 0)
                                         )
#else
                                         "# %5")
#endif
                                     .arg(info.manufacturer())
                                     .arg(info.description())
                                     .arg(info.vendorIdentifier())
                                     .arg(info.productIdentifier())
#if QT_VERSION < QT_VERSION_CHECK(5, 3, 0)
                ;
#else
                                     .arg(info.serialNumber());
#endif
            setItemData(numberDevices, deviceInfo, Qt::ToolTipRole);
        } else {
            setItemData(numberDevices, tr("Not a valid device"), Qt::ToolTipRole);
        }
        numberDevices++;
    }

    int index = findText(selection);
    if (index != -1) {
        // found something - work done!
        setCurrentIndex(index);
    } else {
        // maybe it's no longer connected
        // ToDo add this in a different colour
        // via a own model to indicate the
        // device's absence
        // add this to the list and select this anyway as
        // the user might want to reconnect the device
        // after reconnection the device will be shown in the
        // error/unconnected colour since we will get no notification
        // about devices beeing removed or attached
        // which is our problem in the first place
        addItem(selection);
        setCurrentIndex(numberDevices);
    }
}
