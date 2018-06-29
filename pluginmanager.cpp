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

PluginManager::PluginManager(QVBoxLayout * parent)
    : m_layout(parent)
{

}

void PluginManager::addPlugin(const Plugin * item)
{
    m_list.append(item);
    QMargins mainMargins = m_layout->contentsMargins();
    item->frame->setContentsMargins(mainMargins);
    m_layout->addWidget(item->frame);
    item->frame->show();
    qDebug() << "name: " << item->name;
}

void PluginManager::removePlugin(const Plugin * item)
{
//    m_list.removeOne(item);
}

void PluginManager::processCmd(QString * text)
{
    QListIterator<const Plugin *> i(m_list);
    while (i.hasNext()) {
        const Plugin* item = static_cast<const Plugin*>(i.next());
        if (item->inject && item->processCmd) {
            QString new_text;
            if (item->processCmd(text, &new_text)) {
                *text = new_text;
            }
        }
    }
}

/**
 * @brief Plugins that need to send commands to the serial
 *  port should use this call function and pass the QString.
 *  This function will actually emulate the procedure of
 *  writting the string in the m_input_edit of the
 * @param outStr The string to send to the serial
 */
void PluginManager::proxyCmd(QString * outStr)
{
    emit sendCmd(*outStr);
}
