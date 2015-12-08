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

#include "serialdevicelistmodel.h"

#include <QSerialPortInfo>
#include "qdebug.h"

SerialDeviceListModel::SerialDeviceListModel(QObject *parent)
    : QAbstractListModel(parent)
{
}

int SerialDeviceListModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent)
    qDebug() << Q_FUNC_INFO;
    return m_device_count;
}

QVariant SerialDeviceListModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();

    if (index.row() >= m_devices.size() || index.row() < 0)
        return QVariant();

    qDebug() << Q_FUNC_INFO;
    if (role == Qt::ToolTipRole) {
        return QStringLiteral("SerialDevice ...");
    } else if (role == Qt::DisplayRole) {
        return m_devices.at(index.row());
    } else {
        return QVariant();
    }
}

bool SerialDeviceListModel::canFetchMore(const QModelIndex &parent) const
{
    Q_UNUSED(parent)
    qDebug() << Q_FUNC_INFO;
    return true;
}

void SerialDeviceListModel::fetchMore(const QModelIndex &parent)
{
    Q_UNUSED(parent)
    m_devices.clear();
    for (auto info : QSerialPortInfo::availablePorts()) {
        m_devices.append(info.systemLocation());
    }
    m_device_count = m_devices.size();
}
