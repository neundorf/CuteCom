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

#include "datahighlighter.h"

DataHighlighter::DataHighlighter(QTextDocument *parent)
    : QSyntaxHighlighter(parent)
{
    m_format_time.setForeground(Qt::darkGreen);
    m_pattern_time = new QRegExp("\\d{2,2}:\\d{2,2}:\\d{2,2}:\\d{3,3} ");
    m_format_bytes.setForeground(QColor(120, 180, 200));
    QFont font;
    font.setFamily(font.defaultFamily());
    font.setPointSize(10);
    m_format_bytes.setFont(font);
    m_pattern_bytes = new QRegExp("^\\d{8} ");
    m_format_ctrl.setForeground(Qt::darkRed);
    m_format_ctrl.setFontWeight(QFont::Bold);
    m_pattern_ctrl = new QRegExp("[\\x240A\\x240D\\x21E5]");
    font = QFont("Monospace");
    font.setStyleHint(QFont::Courier);
    font.setPointSize(10);
    m_format_hex.setFont(font);
    m_format_hex.setForeground(Qt::darkMagenta);
    m_pattern_hex = new QRegExp("<0x[\\da-f]{2}>");
    m_format_search.setBackground(QColor(230, 230, 180));
    m_format_search.setForeground(QColor(50, 50, 180));
}

void DataHighlighter::setSearchString(const QString &search)
{
    m_searchString = search;
    rehighlight();
}

void DataHighlighter::setCharFormat(QTextCharFormat *format, DataHighlighter::Formats type)
{
    switch (type) {
    case Formats::HEX:
        // m_format_hex = format;
        break;
    default:
        break;
    }
}

void DataHighlighter::highlightBlock(const QString &text)
{
    if (text.isEmpty())
        return;
    int index = m_pattern_time->indexIn(text);
    if (index >= 0)
        setFormat(index, m_pattern_time->matchedLength(), m_format_time);

    index = m_pattern_bytes->indexIn(text);
    if (index >= 0)
        setFormat(index, m_pattern_bytes->matchedLength(), m_format_bytes);

    index = m_pattern_ctrl->indexIn(text, 0);
    while (index >= 0) {
        setFormat(index, 1, m_format_ctrl);
        index = m_pattern_ctrl->indexIn(text, index + 1);
    }

    index = m_pattern_hex->indexIn(text, 0);
    int l = 0;
    while (index >= 0) {
        l = m_pattern_hex->matchedLength();
        setFormat(index, l, m_format_hex);
        index = m_pattern_hex->indexIn(text, index + l);
    }

    if (m_searchString.isEmpty())
        return;

    const int length = m_searchString.length();
    index = text.indexOf(m_searchString, 0, Qt::CaseInsensitive);
    while (index >= 0) {
        setFormat(index, length, m_format_search);
        index = text.indexOf(m_searchString, index + length, Qt::CaseInsensitive);
    }
}
