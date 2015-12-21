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

#ifndef SESSIONMANAGER_H
#define SESSIONMANAGER_H

#include "ui_sessionmanager.h"
#include "settings.h"

#include <QItemDelegate>

class SessionManager : public QDialog, private Ui::SessionManager
{
    Q_OBJECT

signals:
    void sessionSwitched(const QString &name);
    void sessionRemoved(const QString &name);
    void sessionRenamed(const QString &source, const QString &destination);
    void sessionCloned(const QString &source, const QString &destination);

public:
    explicit SessionManager(Settings *settings, QWidget *parent = 0);
    void editingFinished(const QString &newSessionName);

private:
    void currentItemChanged(QListWidgetItem *current, QListWidgetItem *previous);
    void switchSession();
    void removeSession();
    void cloneSession();
    void renameSession();

    /**
     * stores the reference of the application wide settings instance
     * @brief m_settings
     */
    Settings *m_settings;
    /**
     * The currently selected session
     * @brief m_current_item
     */
    QListWidgetItem *m_current_item;
    /**
     * The currently used session
     * @brief m_current_item
     */
    QListWidgetItem *m_current_session;

    bool m_isCloning;
    bool m_isRenaming;
    QString m_previous_sessionName;
};

class SessionItemDelegate : public QItemDelegate
{
    Q_OBJECT
public:
    SessionItemDelegate(QListWidget *list)
        : QItemDelegate(list)
        , m_list(list)
    {
    }
    QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const Q_DECL_OVERRIDE;

signals:
    void editingFinished(QString newSessionName) const;

private:
    QListWidget *m_list;
};

#endif // SESSIONMANAGER_H
