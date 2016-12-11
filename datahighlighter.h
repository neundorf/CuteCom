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

#ifndef DATAHIGHLIGHTER_H
#define DATAHIGHLIGHTER_H

#include <QSyntaxHighlighter>

class DataHighlighter : public QSyntaxHighlighter
{
    Q_OBJECT

public:
    enum Formats { HEX };

    DataHighlighter(QTextDocument *parent = 0);
    void setSearchString(const QString &search);
    void setCharFormat(QTextCharFormat *format, Formats type);

protected:
    void highlightBlock(const QString &text) Q_DECL_OVERRIDE;

private:
    QRegExp *m_pattern_time;
    QTextCharFormat m_format_time;
    QRegExp *m_pattern_bytes;
    QTextCharFormat m_format_bytes;
    QRegExp *m_pattern_ctrl;
    QTextCharFormat m_format_ctrl;
    QRegExp *m_pattern_hex;
    QTextCharFormat m_format_hex;
    QTextCharFormat m_format_search;

    QString m_searchString;
};

#endif // DATAHIGHLIGHTER_H
