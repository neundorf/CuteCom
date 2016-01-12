/*
 * Copyright (C) 2004-2009 Alexander Neundorf <neundorf@kde.org> (code used from original CuteCom)
 * Copyright (c) 2015 Meinhard Ritscher <cyc1ingsir@gmail.com>
 * Copyright (c) 2015 Antoine Calando <acalando@free.fr> (displaying Ctrl-characters and ascii for hex)
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

#include "datadisplay.h"

#include <QTime>
#include <QScrollBar>
#include <QDebug>

DataDisplay::DataDisplay(QWidget *parent)
    : QTextEdit(parent)
    , m_hexBytes(0)
    , m_displayHex(false)
    , m_displayTime(false)
    , m_displayCtrlCharacters(false)
    , m_timestampFormat(QStringLiteral("HH:mm:ss:zzz"))
    , m_previous_ended_with_nl(true)
{
    setupTextFormats();
}

void DataDisplay::clear()
{
    m_hexBytes = 0;
    QTextEdit::clear();
}

/**
 * Prepare data and finally append it to the end of text edit's
 * view port.
 * @brief OutputTerminal::displayData
 * @param data
 */
void DataDisplay::displayData(const QByteArray &data)
{
    if (m_displayTime) {
        QTime timestamp = QTime::currentTime();
        m_timestamp = QStringLiteral("[") + timestamp.toString(m_timestampFormat) + QStringLiteral("] ");
    }

    if (m_displayHex) {
        if (formatHexData(data)) {
            // the last line was incomplete
            // we remove it from the display before redrawing it
            // with the current data added
            QTextCursor storeCursorPos = textCursor();
            moveCursor(QTextCursor::End, QTextCursor::MoveAnchor);
            moveCursor(QTextCursor::StartOfLine, QTextCursor::MoveAnchor);
            moveCursor(QTextCursor::End, QTextCursor::KeepAnchor);
            textCursor().removeSelectedText();
            // textCursor().deletePreviousChar();
            setTextCursor(storeCursorPos);
        }
    } else if (!data.contains('\n')) {
        constructDisplayLine(data);
    } else {
        // display multiple lines

        // split line at '\n' but do not remove them
        QList<QByteArray> lines;
        int start = 0;
        int end;
        while ((end = data.indexOf('\n', start)) != -1) {
            end++; // include separator
            lines.append(data.mid(start, end - start));
            start = end;
        }
        if (start < data.size()) {
            lines.append(data.mid(start));
            // ends_with_lf = false;
        }

        // if the last line of the previous junk of data
        // was less than 16 bytes of data, it needs to
        // be rewritten
        // in this case, the last line was not removed
        // test if last line of previous junk was complete
        // append the first line of the current junk
        // if (m_displayHex && m_data.size() > 0) {
        //    constructDisplayLine(lines.takeFirst(), true);
        // }

        foreach (QByteArray line, lines) {
            constructDisplayLine(line);
        }
    }

    // stop auto scrolling if the user scrolled to
    // to older data
    QScrollBar *sb = verticalScrollBar();
    int save_scroll = sb->value();
    int save_max = (save_scroll == sb->maximum());

    // append the data to end of the parent's TextEdit
    // each part of the line with it's set format
    foreach (DisplayLine line, m_data) {

        if (m_displayHex) {
            moveCursor(QTextCursor::End);
            mergeCurrentCharFormat(*m_format_prefix);
            insertPlainText(line.prefix);

            moveCursor(QTextCursor::End);
            mergeCurrentCharFormat(*m_format_hex);
            insertPlainText(line.data);
            moveCursor(QTextCursor::End);
            mergeCurrentCharFormat(*m_format_ascii);
            insertPlainText(line.trailer);
        } else {
            if (line.prefix.size() > 0) {
                moveCursor(QTextCursor::End);
                mergeCurrentCharFormat(*m_format_prefix);
                insertPlainText(line.prefix);
            }
            moveCursor(QTextCursor::End);
            mergeCurrentCharFormat(*m_format_data);
            insertPlainText(line.data);
        }
    }
    m_data.clear();

    if (save_max)
        sb->setValue(sb->maximum());
    else
        sb->setValue(save_scroll);
}

/*!
 * Inserts a space between every <step> byte
 * abcdefg => a b c d e f g
 * \brief OutputTerminal::insertSpaces
 * \param data
 * \param step
 */
void DataDisplay::insertSpaces(QString &data, unsigned int step)
{
    for (unsigned int i = data.size() - step; i > 0; i -= step) {
        data.insert(i, ' ');
        if (i == (8 * step)) {
            data.insert(i, QStringLiteral("  "));
        }
    }
}

/*!
 * \brief OutputTerminal::formatHexData
 * \param inData
 * \return should the last line be redisplayed
 */
bool DataDisplay::formatHexData(const QByteArray &inData)
{

    QByteArray data;
    bool redisplay = false;

    // 16 bytes will be displayed on each line
    // if less have been displayed on the last round
    // add to the last line and redisplay
    int overflow = m_hexBytes % 16;
    if (overflow) {
        data = m_hexLeftOver.append(inData);
        m_hexBytes -= overflow;
        redisplay = true;
    } else {
        data = inData;
    }

    QStringList junks;
    int pos = 0;
    QByteArray junk;

    unsigned int junkSize = 0;
    while (pos < data.size()) {
        junk = data.mid(pos, 16);
        junkSize = junk.size();
        QString hexJunk = QString(junk.toHex());
        QString asciiText;
        char *c = junk.data();
        while (*c) {
            unsigned int b = *c;
            if (b < 0x20) {
                b += 0x2400;
            } else if (0x7F <= b) {
                b = '.';
            }
            asciiText += QChar(b);
            ++c;
        }
        if (junkSize == 16)
            asciiText.append('\n');
        insertSpaces(hexJunk, 2);
        if (asciiText.size() > 8)
            asciiText.insert(8, QStringLiteral("  "));

        DisplayLine line;
        line.prefix = QString("%1 ").arg(m_hexBytes, 8, 10, QChar('0'));
        line.data = QString("%1\t").arg(hexJunk, -50);
        line.trailer = QString(asciiText);
        m_data.append(line);
        pos += 16;
        m_hexBytes += junk.size();
    }
    if (junkSize < 16) {
        m_hexLeftOver = junk;
        m_previous_ended_with_nl = false;
    } else {
        m_previous_ended_with_nl = true;
    }

    return redisplay;
}

/*!
 * \brief OutputTerminal::constructDisplayLine
 * \param inData
 */
void DataDisplay::constructDisplayLine(const QByteArray &inData)
{
    DisplayLine line;

    if (m_displayTime && m_previous_ended_with_nl) {
        line.prefix = m_timestamp;
    }

    for (int i = 0; i < inData.size(); i++) {
        unsigned int b = inData.at(i);
        // print one newline for \r\n only
        if ((isprint(b)) || (b == '\n') || (b == '\r') || (b == '\t')) {

            if (b == '\r') {
                if (m_displayCtrlCharacters)
                    line.data += QChar(0x240D);
            } else if (b == '\n') {
                if (m_displayCtrlCharacters)
                    line.data += QChar(0x240A);
                line.data += '\n';
                Q_ASSERT(i != (inData.size()));
            } else if (b == '\t') {
                if (m_displayCtrlCharacters)
                    line.data += QChar(0x21E5);
                line.data += '\t';
            } else {
                line.data += b;
            }

        } else {
            if (b == '\0') {
                line.data += "<break>\n";
                m_previous_ended_with_nl = true;
                m_data.append(line);
                line = DisplayLine();
                line.prefix = m_timestamp;
                continue;
            } else {
                line.data += QString("<0x%1>").arg(b & 0xff, 2, 16, QChar('0'));
            }
        }
    }
    if (!line.data.isEmpty()) {
        m_data.append(line);
        m_previous_ended_with_nl = line.data.endsWith('\n');
    }
}

/*!
 * \brief OutputTerminal::setDisplayTime
 * \param displayTime
 */
void DataDisplay::setDisplayTime(bool displayTime)
{
    m_displayTime = displayTime;
    if (!m_previous_ended_with_nl) {
        displayData(QByteArray(1, '\n'));
    }
}

/*!
 * \brief OutputTerminal::setDisplayHex
 * \param displayHex
 */
void DataDisplay::setDisplayHex(bool displayHex)
{
    if (displayHex) {
        setLineWrapMode(QTextEdit::NoWrap);
        if (!m_previous_ended_with_nl) {
            displayData(QByteArray(1, '\n'));
        }
        m_hexBytes = 0;
        m_displayHex = displayHex;
    } else {
        setLineWrapMode(QTextEdit::WidgetWidth);
        m_displayHex = displayHex;
        if (!m_previous_ended_with_nl) {
            displayData(QByteArray(1, '\n'));
        }
    }
}

/*!
 * \brief OutputTerminal::setDisplayCtrlCharacters
 * \param displayCtrlCharacters
 */
void DataDisplay::setDisplayCtrlCharacters(bool displayCtrlCharacters)
{
    m_displayCtrlCharacters = displayCtrlCharacters;
}

/*!
 * \brief OutputTerminal::setTimestampFormat
 * \param timestampFormat
 */
void DataDisplay::setTimestampFormat(const QString &timestampFormat) { m_timestampFormat = timestampFormat; }

/*!
 * Setting up different formats for displaying
 * different sections of the data differently
 * \brief OutputTerminal::setupTextFormats
 */
void DataDisplay::setupTextFormats()
{
    // ToDo make this changeable via settings

    QTextCursor cursor = textCursor();
    QTextCharFormat format = cursor.charFormat();
    QColor col = QColor(Qt::black);
    format.setForeground(col);
    QFont font;
    font.setFamily(font.defaultFamily());
    font.setPointSize(10);
    format.setFont(font);
    m_format_data = new QTextCharFormat(format);
    qDebug() << m_format_data->foreground();

    col = QColor(120, 180, 200);
    format.setForeground(col);
    m_format_prefix = new QTextCharFormat(format);
    qDebug() << m_format_prefix->foreground();

    col = QColor(Qt::black);
    format.setForeground(col);
    font = QFont("Monospace");
    font.setStyleHint(QFont::Courier);
    font.setPointSize(10);
    //    font.setFixedPitch(true);
    //    font.setKerning(false);
    format.setFont(font);
    m_format_hex = new QTextCharFormat(format);

    col = QColor(100, 100, 100);
    format.setForeground(col);
    m_format_ascii = new QTextCharFormat(format);
}
