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
#include "sessionmanager.h"

SessionManager::SessionManager(Settings *settings, QWidget *parent) :
    QDialog(parent)
    ,m_settings(settings)
{
    setupUi(this);

    m_session_list->addItems(m_settings->getSessionNames());
    QList<QListWidgetItem *> items = m_session_list->findItems(m_settings->getCurrentSessionName(), Qt::MatchExactly);
    if (items.size() > 0) {
        m_session_list->setCurrentItem(items.at(0));
    }

    connect(m_bt_switch, &QPushButton::clicked, this, &SessionManager::switchSession);
}

void SessionManager::switchSession()
{
    QList<QListWidgetItem *> items = m_session_list->selectedItems();
    if(items.size() > 0)
        emit sessionSwitched(items.at(0)->text());

}
