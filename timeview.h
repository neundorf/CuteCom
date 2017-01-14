/*
 * Copyright (c) 2016 Meinhard Ritscher <cyc1ingsir@gmail.com>
 *
 * This is based on the Qt code editor example line numbering view.
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

#ifndef TIMEVIEW_H
#define TIMEVIEW_H

#include <QWidget>
class DataDisplayPrivate;

class TimeView : public QWidget
{
    Q_OBJECT
public:
    explicit TimeView(DataDisplayPrivate *display);

    QSize sizeHint() const Q_DECL_OVERRIDE;

protected:
    void paintEvent(QPaintEvent *event) Q_DECL_OVERRIDE;

private:
    DataDisplayPrivate *dataDisplay;
};

#endif // TIMEVIEW_H
