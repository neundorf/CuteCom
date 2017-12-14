/*
 * Copyright (c) 2015-2016 Meinhard Ritscher <cyc1ingsir@gmail.com>
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

#include <QPlainTextEdit>
#include <QTime>
#include <QTimer>

class TimeView;
class SearchPanel;
class DataDisplayPrivate;
class QAction;
class DataHighlighter;

class DataDisplay : public QWidget
{
    Q_OBJECT

    struct DisplayLine {
        QString data;
        QString trailer;
    };

public:
    explicit DataDisplay(QWidget *parent = 0);

    void clear();

    void setReadOnly(bool readonly);

    void setUndoRedoEnabled(bool enabled);

    void startSearch();

    void displayData(const QByteArray &data);

    void setDisplayTime(bool displayTime);

    void setDisplayHex(bool displayHex);

    void setDisplayCtrlCharacters(bool displayCtrlCharacters);

    void setLinebreakChar(const QString &chars);

    QTextDocument *getTextDocument();

private:
    void find(const QString &, QTextDocument::FindFlags);
    void insertSpaces(QString &data, unsigned int step = 1);
    bool formatHexData(const QByteArray &inData);
    void constructDisplayLine(const QByteArray &inData);
    void setupTextFormats();

    DataDisplayPrivate *m_dataDisplay;

    SearchPanel *m_searchPanel;

    int m_searchAreaHeight;

    /**
     * @brief m_timestamp
     */
    QTime m_timestamp;

    /**
     * @brief m_hexBytes
     */
    quint64 m_hexBytes;

    /**
     * In case the last printed hex line
     * contained less than 16 bytes,
     * The line's raw content will be stored
     * here so it can be reprinted in the next round
     * @brief m_hexLeftOver
     */
    QByteArray m_hexLeftOver;

    /**
     * Data is displayed as hexadecimal values.
     * @brief m_displayHex
     */
    bool m_displayHex;

    /**
     * Ctrl characters like LF and Tabs are visualized
     * with symbols
     * @brief m_displayCtrlCharacters
     */
    bool m_displayCtrlCharacters;

    /**
     * defines characters at which a new line is generated.
     */
    char m_linebreakChar;

    /**
     * The container to store multiple formated
     * lines before beeing printed
     * @brief m_data
     */
    QList<DisplayLine> m_data;

    bool m_previous_ended_with_nl;

    QTextCharFormat *m_format_data;
    QTextCharFormat *m_format_hex;
    QTextCharFormat *m_format_ascii;

    QVector<QTime> *m_timestamps;
    DataHighlighter *m_highlighter;
    bool m_redisplay;
    QTimer m_bufferingIncomingDataTimer;

private slots:
    void displayDataFromBuffer(void);
};

class DataDisplayPrivate : public QPlainTextEdit
{
    Q_OBJECT

public:
    explicit DataDisplayPrivate(DataDisplay *parent = 0);

    void timeViewPaintEvent(QPaintEvent *event);

    int timeViewWidth();

    void setTimestampFormat(const QString &timestampFormat);

    void setTimeFormat(QTextCharFormat *format_time);

    QVector<QTime> *timestamps();

    void setDisplayTime(bool displayTime);

protected:
    void resizeEvent(QResizeEvent *event) Q_DECL_OVERRIDE;
    void updateTimeView(const QRect &, int);

private:
    QTextCharFormat *m_format_time;

    /**
     * @brief m_timestampFormat
     */
    QString m_timestampFormat;

    int m_time_width;
    TimeView *m_timeView;
    QVector<QTime> *m_timestamps;
};

#endif // DATADISPLAY_H
