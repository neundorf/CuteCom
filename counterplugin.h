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

#ifndef COUNTERPLUGIN_H
#define COUNTERPLUGIN_H

#include "plugin.h"
#include "settings.h"
#include <QDebug>
#include <QFrame>

namespace Ui
{
class CounterPlugin;
}

class CounterPlugin : public QFrame
{
    Q_OBJECT

public:
    explicit CounterPlugin(QFrame *parent, Settings *settings);
    ~CounterPlugin();
    const Plugin *plugin();
    //    int processCmd(const QString *text, QString *new_text);

signals:
    void sendCmd(QByteArray);
    void unload(Plugin *);

public slots:
    void txBytes(int);
    void rxBytes(QByteArray);
    void removePlugin(bool);
    void helpMsg(void);

private:
    Ui::CounterPlugin *ui;
    Plugin *m_plugin;
    Settings *m_settings;
};

#endif // COUNTERPLUGIN_H
