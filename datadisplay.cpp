/*
 * Copyright (C) 2004-2009 Alexander Neundorf <neundorf@kde.org> (code used from original CuteCom)
 * Copyright (c) 2015-2016 Meinhard Ritscher <cyc1ingsir@gmail.com>
 * Copyright (c) 2015 Antoine Calando <acalando@free.fr> (displaying Ctrl-characters and ascii for hex)
 * Copyright (c) 2016 Pauli Sundberg <https://github.com/susundberg> (Make several non-printable characters to be
 *grouped)
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
#include "timeview.h"
#include "searchpanel.h"
#include "datahighlighter.h"

#include <QScrollBar>
#include <QPainter>
#include <QTextBlock>
#include <QDebug>

DataDisplay::DataDisplay(QWidget *parent)
    : QWidget(parent)
    , m_dataDisplay(new DataDisplayPrivate(this))
    , m_searchPanel(new SearchPanel(this))
    , m_hexBytes(0)
    , m_hexLeftOver(0)
    , m_displayHex(false)
    , m_displayCtrlCharacters(false)
    , m_linebreakChar('\n')
    , m_previous_ended_with_nl(true)
{
    setupTextFormats();
    m_timestamps = m_dataDisplay->timestamps();
    m_highlighter = new DataHighlighter(m_dataDisplay->document());

    QVBoxLayout *layout = new QVBoxLayout(this);
    // to remove any margin around the layout
    layout->setContentsMargins(QMargins());
    layout->setSpacing(0);

    layout->addWidget(m_dataDisplay);
    layout->addWidget(m_searchPanel);
    m_searchPanel->hide();

    connect(m_searchPanel, &SearchPanel::findNext, this, &DataDisplay::find);
    connect(m_searchPanel, &SearchPanel::textEntered, m_highlighter, &DataHighlighter::setSearchString);

    connect(&m_timer, SIGNAL(timeout()), this, SLOT(BlockReady()));
    m_timer.start(100);
}

void DataDisplay::clear()
{
    m_hexBytes = 0;
    m_timestamps->clear();
    m_dataDisplay->clear();
}

void DataDisplay::setReadOnly(bool readonly) { m_dataDisplay->setReadOnly(readonly); }

void DataDisplay::setUndoRedoEnabled(bool enable) { m_dataDisplay->setUndoRedoEnabled(enable); }

void DataDisplay::BlockReady(void)
{
    if (m_data.isEmpty())
        return;

    // Store selection position before appending new data
    QTextCursor cursor = m_dataDisplay->textCursor();
    const int selStart = cursor.selectionStart();
    const int selLength = cursor.selectionEnd() - selStart;

    // stop auto scrolling if the user scrolled to
    // to older data
    QScrollBar *sb = m_dataDisplay->verticalScrollBar();
    int save_scroll = sb->value();
    bool save_max = (save_scroll == sb->maximum());

    m_dataDisplay->moveCursor(QTextCursor::End);
    if (!m_displayHex) {
        m_dataDisplay->textCursor().beginEditBlock();
        // append the data to end of the parent's TextEdit
        // each part of the line with it's set format
        foreach (DisplayLine line, m_data) {
            m_dataDisplay->textCursor().insertText(line.data, *m_format_data);
        }
        m_dataDisplay->textCursor().endEditBlock();
    } else {
        // the last line was incomplete
        // we remove it from the display before redrawing it
        // with the current data added
        foreach (DisplayLine line, m_data) {
            m_dataDisplay->moveCursor(QTextCursor::End, QTextCursor::MoveAnchor);
            m_dataDisplay->moveCursor(QTextCursor::StartOfLine, QTextCursor::MoveAnchor);
            m_dataDisplay->moveCursor(QTextCursor::End, QTextCursor::KeepAnchor);
            m_dataDisplay->textCursor().removeSelectedText();
            // textCursor().deletePreviousChar();
            //------------------------------------------
            m_dataDisplay->textCursor().insertText(line.data, *m_format_hex);
            m_dataDisplay->textCursor().insertText(line.trailer, *m_format_ascii);
        }
    }
    //        qDebug() << "last TextBlock # " << blockCount() << " length: " << m_timestamps.length();
    //        Q_ASSERT(blockCount() == m_timestamps.length());
    m_data.clear();

    // if any text was selected before appending new data then restore that selection
    if (selLength > 0) {
        // set the anchor - start of the selection
        cursor.setPosition(selStart, QTextCursor::MoveMode::MoveAnchor);
        // set the position - this just selects the text
        cursor.movePosition(QTextCursor::MoveOperation::NextCharacter, QTextCursor::MoveMode::KeepAnchor, selLength);
        m_dataDisplay->setTextCursor(cursor);
    }

    if (save_max)
        sb->setValue(sb->maximum());
    else
        sb->setValue(save_scroll);
}
/*!
 * Prepare data and finally append it to the end of text edit's
 * view port.
 * \brief OutputTerminal::displayData
 * \param data
 */
void DataDisplay::displayData(const QByteArray &data)
{
    m_timestamp = QTime::currentTime();

    if (m_displayHex) {
        formatHexData(data);
    } else if (!data.contains(m_linebreakChar)) {
        constructDisplayLine(data);
    } else {
        // display multiple lines

        // split line at m_linebreakChar (e.g.'\n') but do not remove them
        // ToDo: Testing needed to verify that the display will not be messed up
        // if
        //   - m_linebreakChar is setup with `\r` AND
        //   - line contains '\n' AND
        //   - timeview is enabled :(
        QList<QByteArray> lines;
        int start = 0;
        int end;
        while ((end = data.indexOf(m_linebreakChar, start)) != -1) {
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

    // append the data to end of the parent's TextEdit
    // each part of the line with it's set format
    // moved to BlockReady()
}

/*!
 * \brief OutputTerminal::constructDisplayLine
 * \param inData
 */
void DataDisplay::constructDisplayLine(const QByteArray &inData)
{
    DisplayLine line;

    if (m_previous_ended_with_nl) {
        m_timestamps->append(m_timestamp);
    }

    for (int i = 0; i < inData.size(); i++) {
        unsigned int b = inData.at(i);
        // print newline depending on m_linebreakChar
        if ((isprint(b)) || (b == '\n') || (b == '\r') || (b == '\t')) {

            if (b == '\r') {
                if (m_displayCtrlCharacters)
                    line.data += QChar(0x240D);
                if (m_linebreakChar == '\r') {
                    line.data += '\n';
                }
            } else if (b == '\n') {
                if (m_displayCtrlCharacters)
                    line.data += QChar(0x240A);
                if (m_linebreakChar == '\n') {
                    line.data += '\n';
                }
                Q_ASSERT(i != (inData.size()));
            } else if (b == '\t') {
                if (m_displayCtrlCharacters)
                    line.data += QChar(0x21E5);
                line.data += '\t';
            } else {
                line.data += b;
            }

        } else {

            /* testcases:
             *   0
             *   0000
             *   0x999
             *   0x1024
             *   abc0  plus all above (2-3) with leading abc
             *   0z  plus all above (2-3) with trailing z
             *   abc0z plus all above (2-3) with leading abc and trailing z
             */

            if (b == '\0') {
                // Check if we have multiple zeros here -- concatenate them to a single printe
                int nbreaks = 0;

                for (int nbreaks_loop = i; nbreaks_loop < inData.size(); nbreaks_loop++) {
                    if (inData.at(nbreaks_loop) != (int)b)
                        break;

                    nbreaks += 1;
                    if (nbreaks > 999) { // make sure our padding to 3 characters dont go over
                        line.data += QString("<break x %2>\n").arg(999, 3, 10, QChar('0'));
                        m_data.append(line);
                        line = DisplayLine();
                        m_timestamps->append(m_timestamp);
                        nbreaks -= 999;
                        i += 999;
                    }
                }

                if (nbreaks == 1) {
                    // Nice single '<break>' print
                    line.data += QString("<break>\n");
                } else {
                    // and multi print when needed
                    line.data += QString("<break x %2>\n").arg(nbreaks, 3, 10, QChar('0'));
                    i += (nbreaks - 1);
                }

                if (i < (inData.size() - 1)) {
                    // append line as there are trailing characters after the break
                    m_data.append(line);
                    line = DisplayLine();
                    m_timestamps->append(m_timestamp);
                }
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

void DataDisplay::setDisplayTime(bool displayTime) { m_dataDisplay->setDisplayTime(displayTime); }

/*!
 * \brief OutputTerminal::setDisplayHex
 * \param displayHex
 */
void DataDisplay::setDisplayHex(bool displayHex)
{
    if (displayHex) {
        m_dataDisplay->setLineWrapMode(QPlainTextEdit::NoWrap);
        if (!m_previous_ended_with_nl) {
            displayData(QByteArray(1, '\n'));
        }
        m_hexBytes = 0;
        m_displayHex = displayHex;
    } else {
        m_dataDisplay->setLineWrapMode(QPlainTextEdit::WidgetWidth);
        m_displayHex = displayHex;
        // make sure new data arriving after
        // switching to hex output is being displayed
        // on a new line
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

void DataDisplay::setLinebreakChar(const QString &chars)
{
    if (chars.size() > 0)
        m_linebreakChar = chars.data()[chars.size() - 1].toLatin1();
    else
        m_linebreakChar = '\n';
}

QTextDocument *DataDisplay::getTextDocument() { return m_dataDisplay->document(); }

/*!
 * \brief DataDisplay::find
 */
void DataDisplay::find(const QString &text, QTextDocument::FindFlags flags)
{
    bool found = m_dataDisplay->find(text, flags);
    m_searchPanel->setPatternFound(found);
}

/*!
 * \brief DataDisplay::startSearch
 */
void DataDisplay::startSearch() { m_searchPanel->showPanel(true); }

/*!
 * Setting up different formats for displaying
 * different sections of the data differently
 * \brief OutputTerminal::setupTextFormats
 */
void DataDisplay::setupTextFormats()
{
    // ToDo make this changeable via settings

    QTextCursor cursor = m_dataDisplay->textCursor();
    QTextCharFormat format = cursor.charFormat();
    QColor col = QColor(Qt::black);
    format.setForeground(col);
    QFont font;
    font.setFamily(font.defaultFamily());
    font.setPointSize(10);
    format.setFont(font);
    m_format_data = new QTextCharFormat(format);
    //    qDebug() << m_format_data->foreground();

    col = QColor(100, 100, 200);
    format.setForeground(col);
    m_dataDisplay->setTimeFormat(new QTextCharFormat(format));

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
        line.data = QString("%1 %2\t").arg(m_hexBytes, 8, 10, QChar('0')).arg(hexJunk, -50);
        line.trailer = QString(asciiText);
        if (!redisplay || pos > 0)
            m_timestamps->append(m_timestamp);
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

/* ****************************************************************************************************
 *
 *                  P R I V A T E
 *
 * *************************************************************************************************** */

/*!
 * \brief DataDisplayPrivate::DataDisplayPrivate
 * \param parent
 */
DataDisplayPrivate::DataDisplayPrivate(DataDisplay *parent)
    : QPlainTextEdit(parent)
    , m_timestampFormat(QStringLiteral("HH:mm:ss:zzz"))
    , m_time_width(0)
    , m_timeView(new TimeView(this))
{
    m_timestamps = new QVector<QTime>();
    connect(this, &QPlainTextEdit::updateRequest, this, &DataDisplayPrivate::updateTimeView);
}

/*!
 * overiden function from QPlainTextEdit::resizeEvent()
 * \brief DataDisplayPrivate::resizeEvent
 * \param event
 */
void DataDisplayPrivate::resizeEvent(QResizeEvent *event)
{
    QPlainTextEdit::resizeEvent(event);

    QRect cr = contentsRect();
    m_timeView->setGeometry(QRect(cr.left(), cr.top(), m_time_width, cr.height()));
}

/*!
 * Displaying the timestamps for each line in a seperate
 * viewport left of the data display
 * \brief DataDisplayPrivate::timeViewPaintEvent
 * \param event
 */
void DataDisplayPrivate::timeViewPaintEvent(QPaintEvent *event)
{
    QPainter painter(m_timeView);
    painter.fillRect(event->rect(), QColor(233, 233, 233));
    painter.setPen(m_format_time->foreground().color());
    painter.setFont(m_format_time->font());
    QTextBlock block = firstVisibleBlock();
    int blockNumber = block.blockNumber();
    int top = (int)blockBoundingGeometry(block).translated(contentOffset()).top();
    int bottom = top + (int)blockBoundingRect(block).height();

    while (block.isValid() && top <= event->rect().bottom()) {
        if (block.isVisible() && bottom >= event->rect().top()) {
            QString time;
            if (m_timestamps->length() > blockNumber) {
                time = m_timestamps->at(blockNumber).toString(m_timestampFormat);
            }
            painter.drawText(0, top, m_timeView->width(), fontMetrics().height(), Qt::AlignRight, time);
        }

        block = block.next();
        top = bottom;
        bottom = top + (int)blockBoundingRect(block).height();
        ++blockNumber;
    }
}

/*!
 * This function is invoked when the displays viewport has been scrolled
 * \brief DataDisplayPrivate::updateTimeView
 * \param rect
 * \param dy
 */
void DataDisplayPrivate::updateTimeView(const QRect &rect, int dy)
{
    if (dy)
        m_timeView->scroll(0, dy);
    else
        m_timeView->update(0, rect.y(), m_timeView->width(), rect.height());

    //    if (rect.contains(viewport()->rect()))
    //        setViewportMargins(m_prefix_width,0,0,0);
}

int DataDisplayPrivate::timeViewWidth() { return m_time_width; }

/*!
 * \brief OutputTerminal::setDisplayTime
 * \param displayTime
 */
void DataDisplayPrivate::setDisplayTime(bool displayTime)
{
    if (displayTime) {
        QFontMetrics *metric = new QFontMetrics(m_format_time->font());
        m_time_width = 3 + metric->width(QStringLiteral("00:00:00:000"));
    } else {
        m_time_width = 0;
    }
    setViewportMargins(m_time_width, 0, 0, 0);
}

QVector<QTime> *DataDisplayPrivate::timestamps() { return m_timestamps; }

void DataDisplayPrivate::setTimeFormat(QTextCharFormat *format_time) { m_format_time = format_time; }

/*!
 * \brief OutputTerminal::setTimestampFormat
 * \param timestampFormat
 */
void DataDisplayPrivate::setTimestampFormat(const QString &timestampFormat) { m_timestampFormat = timestampFormat; }
