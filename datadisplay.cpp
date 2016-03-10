/*
 * Copyright (C) 2004-2009 Alexander Neundorf <neundorf@kde.org> (code used from original CuteCom)
 * Copyright (c) 2015-2016 Meinhard Ritscher <cyc1ingsir@gmail.com>
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
#include "timeview.h"
#include "searchpanel.h"

#include <QScrollBar>
#include <QPainter>
#include <QTextBlock>
#include <QDebug>

DataDisplay::DataDisplay(QWidget *parent)
    : QWidget(parent)
    , m_dataDisplay(new DataDisplayPrivate(this))
    , m_searchPanel(new SearchPanel(this))
    , m_searchAreaHeight(0)
    , m_hexBytes(0)
    , m_hexLeftOver(0)
    , m_displayHex(false)
    , m_displayCtrlCharacters(false)
    , m_previous_ended_with_nl(true)
{
    setupTextFormats();
    m_dataDisplay->setPrefixFormat(m_format_prefix);
    m_timestamps = m_dataDisplay->timestamps();

    QVBoxLayout *layout = new QVBoxLayout(this);
    // to remove any margin around the layout
    layout->setContentsMargins(QMargins());
    layout->setSpacing(0);

    layout->addWidget(m_dataDisplay);
    layout->addWidget(m_searchPanel);
    m_searchPanel->hide();

    findAction = new QAction(this);
    findAction->setShortcut(QKeySequence::Find);
    this->addAction(findAction);
    connect(findAction, &QAction::triggered, [=]() { showSearchPanel(true); });
    connect(m_searchPanel, &SearchPanel::closing, [=]() { showSearchPanel(false); });
    connect(m_searchPanel, &SearchPanel::findNext, m_dataDisplay,
            static_cast<bool (QPlainTextEdit::*)(const QString &, QTextDocument::FindFlags)>(&QPlainTextEdit::find));
}

void DataDisplay::clear()
{
    m_hexBytes = 0;
    m_timestamps->clear();
    m_dataDisplay->clear();
}

void DataDisplay::setReadOnly(bool readonly) { m_dataDisplay->setReadOnly(readonly); }

void DataDisplay::setUndoRedoEnabled(bool enable) { m_dataDisplay->setUndoRedoEnabled(enable); }

void DataDisplay::showSearchPanel() { m_searchPanel->showPanel(true); }

/*!
 * Prepare data and finally append it to the end of text edit's
 * view port.
 * \brief OutputTerminal::displayData
 * \param data
 */
void DataDisplay::displayData(const QByteArray &data)
{
    m_timestamp = QTime::currentTime();

    // stop auto scrolling if the user scrolled to
    // to older data
    QScrollBar *sb = m_dataDisplay->verticalScrollBar();
    int save_scroll = sb->value();
    int save_max = (save_scroll == sb->maximum());

    if (m_displayHex) {
        if (formatHexData(data)) {
            // the last line was incomplete
            // we remove it from the display before redrawing it
            // with the current data added
            QTextCursor storeCursorPos = m_dataDisplay->textCursor();
            m_dataDisplay->moveCursor(QTextCursor::End, QTextCursor::MoveAnchor);
            m_dataDisplay->moveCursor(QTextCursor::StartOfLine, QTextCursor::MoveAnchor);
            m_dataDisplay->moveCursor(QTextCursor::End, QTextCursor::KeepAnchor);
            m_dataDisplay->textCursor().removeSelectedText();
            // textCursor().deletePreviousChar();
            m_dataDisplay->setTextCursor(storeCursorPos);
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

    // append the data to end of the parent's TextEdit
    // each part of the line with it's set format
    foreach (DisplayLine line, m_data) {

        if (m_displayHex) {
            m_dataDisplay->moveCursor(QTextCursor::End);
            m_dataDisplay->textCursor().insertText(line.prefix, *m_format_prefix);

            m_dataDisplay->moveCursor(QTextCursor::End);
            m_dataDisplay->textCursor().insertText(line.data, *m_format_hex);

            m_dataDisplay->moveCursor(QTextCursor::End);
            m_dataDisplay->textCursor().insertText(line.trailer, *m_format_ascii);
        } else {
            if (line.prefix.size() > 0) {
                m_dataDisplay->moveCursor(QTextCursor::End);
                m_dataDisplay->textCursor().insertText(line.prefix, *m_format_prefix);
            }
            m_dataDisplay->moveCursor(QTextCursor::End);
            m_dataDisplay->textCursor().insertText(line.data, *m_format_data);
        }
        //        qDebug() << "last TextBlock # " << blockCount() << " length: " << m_timestamps.length();
        //        Q_ASSERT(blockCount() == m_timestamps.length());
    }
    m_data.clear();

    if (save_max)
        sb->setValue(sb->maximum());
    else
        sb->setValue(save_scroll);
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
            // Check if we have multiple zeros here -- concatenate them to a single printe
            int nbreaks = 0;
            
            QString to_print;
            
            if (b == '\0') {
               to_print="break";
            }
            else {
               to_print = QString("0x%1").arg(b & 0xff, 2, 16, QChar('0'));
            }
            
            for ( int nbreaks_loop = i; nbreaks_loop < inData.size(); nbreaks_loop++)             {
               if ( inData.at( nbreaks_loop ) != (int)b )
                  break;
               
               nbreaks += 1;
               if ( nbreaks > 999 ) {  // make sure our padding to 3 characters dont go over
                  line.data += QString("<%1 x %2>\n").arg( to_print ).arg( 999, 3, 10, QChar('0') );
                  nbreaks -= 999;
               }
               
            }

            if ( nbreaks == 1 ) {
               // Nice single '<break>' print
               line.data += QString("<%1>\n").arg( to_print );
            }
            else  { 
               // and multi print when needed
               line.data += QString("<%1 x %2>\n").arg( to_print ).arg( nbreaks, 3, 10, QChar('0') );
               i += (nbreaks - 1);
            }
            
            m_previous_ended_with_nl = true;
            m_data.append(line);
            line = DisplayLine();
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

/*!
 * \brief DataDisplay::showSearchPanel
 * \param visible
 */
void DataDisplay::showSearchPanel(bool visible)
{
    if (visible) {
        m_searchAreaHeight = m_searchPanel->height();
    } else {
        m_searchAreaHeight = 0;
    }
    m_searchPanel->showPanel(visible);
}

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

    col = QColor(120, 180, 200);
    format.setForeground(col);
    m_format_prefix = new QTextCharFormat(format);
    //    qDebug() << m_format_prefix->foreground();

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
        line.prefix = QString("%1 ").arg(m_hexBytes, 8, 10, QChar('0'));
        line.data = QString("%1\t").arg(hexJunk, -50);
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
    , m_prefix_width(0)
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
    m_timeView->setGeometry(QRect(cr.left(), cr.top(), m_prefix_width, cr.height()));
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
    painter.fillRect(event->rect(), QColor(233,233,233));
    painter.setPen(m_format_prefix->foreground().color());
    painter.setFont(m_format_prefix->font());
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

int DataDisplayPrivate::timeViewWidth() { return m_prefix_width; }

/*!
 * \brief OutputTerminal::setDisplayTime
 * \param displayTime
 */
void DataDisplayPrivate::setDisplayTime(bool displayTime)
{
    if (displayTime) {
        QFontMetrics *metric = new QFontMetrics(m_format_prefix->font());
        m_prefix_width = 3 + metric->width(QStringLiteral("00:00:00:000"));
    } else {
        m_prefix_width = 0;
    }
    setViewportMargins(m_prefix_width, 0, 0, 0);
}

QVector<QTime> *DataDisplayPrivate::timestamps() { return m_timestamps; }

void DataDisplayPrivate::setPrefixFormat(QTextCharFormat *format_prefix) { m_format_prefix = format_prefix; }

/*!
 * \brief OutputTerminal::setTimestampFormat
 * \param timestampFormat
 */
void DataDisplayPrivate::setTimestampFormat(const QString &timestampFormat) { m_timestampFormat = timestampFormat; }
