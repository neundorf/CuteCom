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
  ,m_current_item(0)
  ,m_current_session(0)
{
    setupUi(this);

    connect(m_session_list, &QListWidget::currentItemChanged, this, &SessionManager::currentItemChanged);

    // special session "Default" is always on top of the list
    m_session_list->addItem(QStringLiteral("Default"));

    QStringList sessions = m_settings->getSessionNames();
    if(sessions.size() > 0) {
        if(sessions.contains(QStringLiteral("Default"))) {
            sessions.removeOne(QStringLiteral("Default"));
        }
        sessions.sort();
        m_session_list->addItems(sessions);
    }
    QList<QListWidgetItem *> items = m_session_list->findItems(m_settings->getCurrentSessionName(), Qt::MatchExactly);
    if (items.size() > 0) {
        m_session_list->setCurrentItem(items.at(0));
        QFont font = m_current_item->font();
        font.setBold(true);
        m_current_item->setFont(font);
    }

    m_current_session = m_current_item;
    m_bt_switch->setEnabled(false);
    connect(m_bt_switch, &QPushButton::clicked, this, &SessionManager::switchSession);
    connect(m_bt_delete, &QPushButton::clicked, this, &SessionManager::removeSession);

}

void SessionManager::currentItemChanged(QListWidgetItem *current, QListWidgetItem *previous)
{
    Q_UNUSED(previous)

    if(current != m_current_session)
        m_bt_switch->setEnabled(true);

    m_current_item = current;
    if(current->text() == QStringLiteral("Default")){
        m_bt_delete->setEnabled(false);
    } else {
        m_bt_delete->setEnabled(true);
    }
}

void SessionManager::switchSession()
{
    m_bt_switch->setEnabled(false);
    emit sessionSwitched(m_current_item->text());
    QFont font = m_current_item->font();
    if(m_current_session)
        m_current_session->setFont(font);
    font.setBold(true);
    m_current_item->setFont(font);

    m_current_session = m_current_item;
}

/**
 * @brief SessionManager::removeSession
 */
void SessionManager::removeSession()
{
    emit sessionRemoved(m_current_item->text());
    QListWidgetItem *remove_item = m_current_item;

    if(m_current_session == remove_item) {
        // the currently used session was removed
        // we would be better off, switching to a different one
        // the default session is save to switch too
        m_session_list->setCurrentRow(0);
        switchSession();
    } else {
        // otherwise we select the current session
        m_session_list->setCurrentRow(m_session_list->row(m_current_session));
        m_bt_switch->setEnabled(false);
    }
    m_session_list->takeItem(m_session_list->row(remove_item));
}
