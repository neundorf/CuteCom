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

#include <QItemDelegate>
#include <QLineEdit>
#include <QValidator>
#include <QWidget>

/**
 * This Validator prevents session names to contain some
 * characters not very sensible to use within session names.
 * It prevents duplicate session names as well.
 * It's based on the one included in the QtCreator's sessiondialog.
 * Copyright (C) 2015 The Qt Company Ltd.
 * @brief The SessionNameValidator class
 */
class SessionNameValidator : public QValidator
{
public:
    SessionNameValidator(QListWidget *list);
    void fixup(QString &input) const;
    QValidator::State validate(QString &input, int &pos) const;

private:
    QListWidget *m_list;
    QString m_original_name;
};

SessionNameValidator::SessionNameValidator(QListWidget *list)
    : QValidator(list)
    , m_list(list)
{
    m_original_name = list->currentItem()->text();
}

QValidator::State SessionNameValidator::validate(QString &input, int &pos) const
{
    Q_UNUSED(pos)

    if (input.contains(QLatin1Char('/')) || input.contains(QLatin1Char(':')) || input.contains(QLatin1Char('\\'))
        || input.contains(QLatin1Char('?')) || input.contains(QLatin1Char('*')) || input.contains(QLatin1Char(' ')))
        return QValidator::Invalid;

    QList<QListWidgetItem *> items = m_list->findItems(input, Qt::MatchExactly);

    //    qDebug() << Q_FUNC_INFO << input << "found "
    //             << items.size() << " x"
    //             << "origin: " << m_original_name;

    if (items.size() > 0 && input != m_original_name) {
        return QValidator::Intermediate;
    } else
        return QValidator::Acceptable;
}

void SessionNameValidator::fixup(QString &input) const
{
    int orderNum = -1;
    QString copy;
    do {
        int index = input.lastIndexOf("-");
        if (index > -1) {
            bool ok;
            if (orderNum < 0)
                orderNum = input.right(input.length() - 1 - index).toInt(&ok);
            if (ok) {
                orderNum++;
                copy = input.left(index + 1) + QString::number(orderNum);
                continue;
            }
        }
        if (orderNum < 0)
            orderNum = 0;
        copy = input + QStringLiteral("-%1").arg(++orderNum);
        //        qDebug() << "fixup   : " << "copy: " << copy << " : " << m_list->findItems(copy,
        //        Qt::MatchExactly).size();
    } while ((m_list->findItems(copy, Qt::MatchExactly)).size() > 0);

    input = copy;
}

QWidget *SessionItemDelegate::createEditor(QWidget *parent, const QStyleOptionViewItem &option,
                                           const QModelIndex &index) const
{
    QWidget *editor = QItemDelegate::createEditor(parent, option, index);
    QLineEdit *lineEdit = dynamic_cast<QLineEdit *>(editor);
    if (lineEdit) {
        lineEdit->setValidator(new SessionNameValidator(m_list));
        connect(lineEdit, &QLineEdit::editingFinished, this, [=]() { emit editingFinished(lineEdit->text()); });
    }
    return editor;
}

SessionManager::SessionManager(Settings *settings, QWidget *parent)
    : QDialog(parent)
    , m_settings(settings)
    , m_current_item(0)
    , m_current_session(0)
    , m_isCloning(false)
    , m_isRenaming(false)
{
    setupUi(this);

    m_session_list->setEditTriggers(QAbstractItemView::NoEditTriggers);

    connect(m_session_list, &QListWidget::currentItemChanged, this, &SessionManager::currentItemChanged);

    // special session "Default" is always on top of the list
    m_session_list->addItem(QStringLiteral("Default"));

    QStringList sessions = m_settings->getSessionNames();
    if (sessions.size() > 0) {
        if (sessions.contains(QStringLiteral("Default"))) {
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
    connect(m_bt_rename, &QPushButton::clicked, this, &SessionManager::renameSession);
    connect(m_bt_clone, &QPushButton::clicked, this, &SessionManager::cloneSession);

    m_session_list->setItemDelegate(new SessionItemDelegate(m_session_list));
    connect(static_cast<SessionItemDelegate *>(m_session_list->itemDelegate()), &SessionItemDelegate::editingFinished,
            this, &SessionManager::editingFinished);
}

void SessionManager::currentItemChanged(QListWidgetItem *current, QListWidgetItem * /*previous*/)
{

    if (current != m_current_session)
        m_bt_switch->setEnabled(true);

    m_current_item = current;
    if (current->text() == QStringLiteral("Default")) {
        m_bt_delete->setEnabled(false);
        m_bt_rename->setEnabled(false);
    } else {
        m_bt_delete->setEnabled(true);
        m_bt_rename->setEnabled(true);
    }
}

void SessionManager::editingFinished(const QString &newSessionName)
{
    //    qDebug() << Q_FUNC_INFO << "New session name: " << newSessionName;
    if (m_isRenaming) {
        if (newSessionName != m_previous_sessionName)
            emit sessionRenamed(m_previous_sessionName, newSessionName);
        if (m_previous_sessionName == m_current_session->text())
            emit sessionSwitched(newSessionName);
        m_isRenaming = false;
    } else if (m_isCloning) {
        emit sessionCloned(m_previous_sessionName, newSessionName);
        m_isCloning = false;
    } else {
        qDebug() << "This slot is called multiple times after editing finished - Why?";
    }
    m_bt_rename->setEnabled(true);
    m_bt_clone->setEnabled(true);
}

void SessionManager::switchSession()
{
    m_bt_switch->setEnabled(false);
    emit sessionSwitched(m_current_item->text());
    QFont font = m_current_item->font();
    if (m_current_session)
        m_current_session->setFont(font);
    font.setBold(true);
    m_current_item->setFont(font);

    m_current_session = m_current_item;
    this->close();
}

/**
 * @brief SessionManager::removeSession
 */
void SessionManager::removeSession()
{
    emit sessionRemoved(m_current_item->text());
    QListWidgetItem *remove_item = m_current_item;

    if (m_current_session == remove_item) {
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

/**
 * @brief SessionManager::cloneSession
 */
void SessionManager::cloneSession()
{
    QString newName = m_current_item->text();
    SessionNameValidator *validator = new SessionNameValidator(m_session_list);
    validator->fixup(newName);
    QListWidgetItem *new_item = new QListWidgetItem(newName, m_session_list);
    new_item->setFlags(new_item->flags() | Qt::ItemIsEditable);

    m_session_list->setCurrentItem(new_item);
    m_isCloning = true;
    m_bt_clone->setEnabled(false);
    m_session_list->editItem(m_current_item);
}
/**
 * @brief SessionManager::renameSession
 */
void SessionManager::renameSession()
{
    m_previous_sessionName = m_current_item->text();
    m_current_item->setFlags(m_current_item->flags() | Qt::ItemIsEditable);

    m_isRenaming = true;
    m_bt_rename->setEnabled(false);
    m_session_list->editItem(m_current_item);
}
