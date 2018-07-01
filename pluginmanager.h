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

/**
 * The plugin manager handles the application plugins. The mainwindow form
 * has an empty dedicated QVBoxLayout that can be used from plugins to show
 * them selves in the main form. If a plugin requires a lot of space then
 * you can use a dedicated window and only add a very small ui element that
 * spawns that dialog. Finaly, the element from the mainwindow that is also
 * used from the plugin manager is the the 'Plugins' dropbox menu item. Every
 * plugin has an action on that menu and every time that the action then a
 * new plugin object will be added; therefore, try to write plugins in a way
 * that multiple instances can be loaded!
 *
 * The pluging manager provides plugins with the finctionality to be able to
 * change the content of the serial cmd string before it's send on the serial
 * port by using the function `processCmd()`. Because there can be several
 * plugins that may be able to modify this string, be aware that the priority
 * order is the same with the index of the plugin in the plugin QList (m_list).
 * Also every pluging can send a serial command with the `sendCmd()` signal.
 *
 * Make sure that plugins clean up themselves properly when unloaded.
*/

#ifndef PLUGINMANAGER_H
#define PLUGINMANAGER_H

#include <QDebug>
#include <QObject>
#include <QFrame>
#include <QVBoxLayout>
#include "settings.h"
#include "plugin.h"
#include "macroplugin.h"
#include "netproxyplugin.h"

class PluginManager : public QObject
{
    Q_OBJECT

public:
    /* Every plug in has its own type */
    enum en_plugin_type {
        PLUGIN_TYPE_MACROS,
        PLUGIN_TYPE_NET_PROXY
    };
    PluginManager(QFrame * parent, QVBoxLayout * layout, Settings * settings);
    virtual ~PluginManager();

public slots:
    void addPluginType(en_plugin_type);
    void removePlugin(Plugin*);

signals:
    void recvCmd(QByteArray);   /* mainwindow -> manager */
    void sendCmd(QString);      /* manager -> mainwindow */

protected:
    void addPlugin(Plugin * item);
    void processCmd(QString * text);

private:
    QFrame * m_parent;
    QVBoxLayout * m_layout;
    QList<Plugin*> m_list;
    Settings * m_settings;
    /* Supported plugins */
    MacroPlugin * m_macro_plugin;
};
#endif // PLUGINMANAGER_H
