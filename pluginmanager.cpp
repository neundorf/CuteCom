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

#include "pluginmanager.h"

#define TRACE                                                                                                          \
    if (!debug) {                                                                                                      \
    } else                                                                                                             \
        qDebug()
static bool debug = false;

PluginManager::PluginManager(QFrame *parent, QVBoxLayout *layout, Settings *settings)
    : m_parent(parent)
    , m_layout(layout)
    , m_settings(settings)
{
}

PluginManager::~PluginManager()
{
    QListIterator<Plugin *> i(m_list);
    while (i.hasNext()) {
        Plugin *item = static_cast<Plugin *>(i.next());
        removePlugin(item);
    }
}

/**
 * @brief [SLOT] Add a new plugin and initialize it depending its type
 * @param type Supported plugin type (see: en_plugin_type)
 */
void PluginManager::addPluginType(en_plugin_type type)
{
    TRACE << "[PluginManager] Adding new plugin: " << type;
    if (type == en_plugin_type::PLUGIN_TYPE_MACROS) {
        /* specific plugin initialization */
        MacroPlugin *macro = new MacroPlugin(m_parent, m_settings);
        connect(macro, SIGNAL(unload(Plugin *)), this, SLOT(removePlugin(Plugin *)));
        connect(macro, SIGNAL(sendCmd(QByteArray)), this, SIGNAL(sendCmd(QByteArray)));
        /* common plugin initialization */
        addPlugin((Plugin *)macro->plugin());
    } else if (type == en_plugin_type::PLUGIN_TYPE_NET_PROXY) {
        NetProxyPlugin *proxy = new NetProxyPlugin(m_parent, m_settings);
        connect(proxy, SIGNAL(unload(Plugin *)), this, SLOT(removePlugin(Plugin *)));
        connect(proxy, SIGNAL(sendCmd(QByteArray)), this, SIGNAL(sendCmd(QByteArray)));
        connect(this, SIGNAL(recvCmd(QByteArray)), proxy, SIGNAL(proxyCmd(QByteArray)));
        /* common plugin initialization */
        addPlugin((Plugin *)proxy->plugin());
    }
}

/**
 * @brief [SLOT] Remove an existing plugin
 * @param plugin A pointer to the plugin to delete
 */
void PluginManager::removePlugin(Plugin *plugin)
{
    TRACE << "[PluginManager] Removing plugin: " << plugin->name;
    if (!plugin)
        return;

    if (plugin->frame) {
        plugin->frame->close();
    }
    plugin->deleteLater();
}

/**
 * @brief Adds a plugin in the manager list and also does the
 *  additional initialization
 * @param item The plugin to add
 */
void PluginManager::addPlugin(Plugin *item)
{
    if (!item)
        return;

    m_list.append(item);
    /* if the plugin has also a frame then add it */
    if (item->frame) {
        QMargins mainMargins = m_layout->contentsMargins();
        item->frame->setContentsMargins(mainMargins);
        m_layout->addWidget(item->frame);
        item->frame->show();
    }
    TRACE << "[PluginManager] Added new plugin: " << item->name;
}

/**
 * @brief Inject and process the cmd data before they sent
 * @param cmd The data
 */
void PluginManager::processCmd(QString *cmd)
{
    TRACE << "[PluginManager] process: " << cmd;
    QListIterator<Plugin *> i(m_list);
    while (i.hasNext()) {
        const Plugin *item = static_cast<const Plugin *>(i.next());
        if (item->processCmd) {
            QString new_cmd;
            if (item->processCmd(cmd, &new_cmd)) {
                //                new_cmd = cmd + QString("_ADD");
                *cmd = new_cmd;
            }
        }
    }
}
