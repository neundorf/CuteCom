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

#ifndef DATADISPLAY_H
#define DATADISPLAY_H

#include <QTextEdit>

class DataDisplay : public QTextEdit
{
    Q_OBJECT

    struct DisplayLine
    {
        QString prefix;
        QString data;
        QString trailer;
    };

public:
    explicit DataDisplay(QWidget *parent = 0);

    void clear();

    void displayData(const QByteArray &data);

    void setDisplayTime(bool displayTime);

    void setDisplayHex(bool displayHex);

    void setDisplayCtrlCharacters(bool displayCtrlCharacters);

    void setTimestampFormat(const QString &timestampFormat);

private:
    void insertSpaces(QString &data, unsigned int step = 1);
    bool formatHexData(const QByteArray &inData);
    void constructDisplayLine(const QByteArray &inData);
    void setupTextFormats();

    /**
     * @brief m_hexBytes
     */
    quint64 m_hexBytes;

    /**
     * Data is displayed as hexadecimal values.
     * @brief m_displayTime
     */
    bool m_displayHex;
    /**
     * Each line is prefixed with a timestamp.
     * @brief m_displayHex
     */
    bool m_displayTime;

    /**
     * Ctrl characters like LF and Tabs are visualized
     * with symbols
     * @brief m_displayCtrlCharacters
     */
    bool m_displayCtrlCharacters;

    /**
     * @brief m_timestampFormat
     */
    QString m_timestampFormat;

    /**
     * @brief m_timestamp
     */
    QString m_timestamp;

    /**
     * The container to store multiple formated
     * lines before beeing printed
     * @brief m_data
     */
    QList<DisplayLine> m_data;

    /**
     * In case the last printed hex line
     * contained less than 16 bytes,
     * The line's raw content will be stored
     * here so it can be reprinted in the next round
     * @brief m_hexLeftOver
     */
    QByteArray m_hexLeftOver;

    bool m_previous_ended_with_nl;

    QTextCharFormat *m_format_prefix;
    QTextCharFormat *m_format_data;
    QTextCharFormat *m_format_hex;
    QTextCharFormat *m_format_ascii;
};

#endif // DATADISPLAY_H
