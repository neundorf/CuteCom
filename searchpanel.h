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

#ifndef SEARCHPANEL_H
#define SEARCHPANEL_H

#include "ui_searchpanel.h"
#include <QTextDocument>

class SearchPanel : public QWidget, private Ui::SearchPanel
{
    Q_OBJECT

public:
    explicit SearchPanel(QWidget *parent = 0);
    void showPanel(bool setVisible);

protected:
    bool eventFilter(QObject *obj, QEvent *event);

signals:
    void closing();
    void findNext(QString searchText, QTextDocument::FindFlags);
};

#endif // SEARCHPANEL_H
