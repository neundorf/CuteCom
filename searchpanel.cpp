/*
 * Copyright (c) 2016 Meinhard Ritscher <cyc1ingsir@gmail.com>
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

#include "searchpanel.h"
#include <QKeyEvent>

SearchPanel::SearchPanel(QWidget *parent)
    : QWidget(parent)
{
    setupUi(this);
    le_searchText->setPlaceholderText(tr("with no selection, search is started at end of text"));
    connect(btn_close, &QToolButton::clicked, [=]() {
        emit closing();
        showPanel(false);
    });
    connect(btn_next, &QToolButton::clicked, [=]() { emit findNext(le_searchText->text(), 0); });
    connect(btn_prev, &QToolButton::clicked,
            [=]() { emit findNext(le_searchText->text(), QTextDocument::FindBackward); });
    installEventFilter(this);
    le_searchText->installEventFilter(this);
    m_original_format = le_searchText->styleSheet();
}

/*!
 * Shows and hides the search panel
 * \brief SearchPanel::showPanel
 * \param visible
 */
void SearchPanel::showPanel(bool visible)
{
    if (!visible) {
        hide();
    } else {
        le_searchText->setFocus();
        if (!le_searchText->text().isEmpty())
            le_searchText->selectAll();
        if (isVisible())
            return;
        setVisible(true);
    }
}

/*!
 * Changes appearance of the font to indicate if
 * the search pattern is found or not
 * \brief SearchPanel::setFound
 * \param patternFound
 */
void SearchPanel::setPatternFound(bool patternFound)
{
    le_searchText->setStyleSheet(((patternFound) ? m_original_format : QStringLiteral("color: red;")));
}

bool SearchPanel::eventFilter(QObject *obj, QEvent *event)
{
    if (event->type() == QEvent::KeyPress) {
        QKeyEvent *ke = (QKeyEvent *)event;

        if (ke->key() == Qt::Key_F3 && !le_searchText->text().isEmpty()) {
            if (ke->modifiers() == Qt::NoModifier) {
                emit findNext(le_searchText->text(), 0);
            } else if (ke->modifiers() == Qt::ShiftModifier) {
                emit findNext(le_searchText->text(), QTextDocument::FindBackward);
            }
        } else if (ke->modifiers() == Qt::NoModifier) {
            if (ke->key() == Qt::Key_Escape) {
                showPanel(false);
                return true;
            } else if (obj == le_searchText) {
                if (ke->key() == Qt::Key_Return) {
                    emit textEntered(le_searchText->text());
                }
            }
        }
    } else {
        return QWidget::eventFilter(obj, event);
    }
    return false;
}
