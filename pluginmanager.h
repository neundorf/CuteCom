/*
 * Copyright (c) 208 Dimitris Tassopoulos <dimtass@gmail.com>
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

#ifndef PLUGINMANAGER_H
#define PLUGINMANAGER_H

#include <QDebug>
#include <QObject>
#include <QVBoxLayout>
#include "plugin.h"


class PluginManager : public QObject
{
    Q_OBJECT

public:

    PluginManager(QVBoxLayout * layout);

    void addPlugin(const Plugin * item);
    void removePlugin(const Plugin * item);
    void process(QString * text);

private:
    QVBoxLayout * m_layout;
    QList<const Plugin*> m_list;
};
#endif // PLUGINMANAGER_H
