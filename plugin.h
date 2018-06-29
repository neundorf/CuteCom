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

#ifndef PLUGIN_H
#define PLUGIN_H

#include <QFrame>
#include <QString>

class Plugin {

public:
    typedef int (*processCmd_fp)(const QString * text, QString * new_text);
    Plugin(QString name, bool enable = false, bool inject = false,
           QFrame * frame = NULL, processCmd_fp fp = NULL)
        : name(name), enable(enable), inject(inject), frame(frame), processCmd(fp) {}

    QString name;
    bool enable;
    bool inject;
    QFrame * frame;
    processCmd_fp processCmd;
};

#endif // PLUGIN_H
