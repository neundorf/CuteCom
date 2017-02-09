/*
 * Copyright (c) 2017 Slawomir Pabian <sla.pab.dev@gmail.com>
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

#include "ctrlcharacterspopup.h"
#include <QApplication>
#include <QKeyEvent>
#include <QPainter>
#include <QPoint>
#include <QPushButton>
#include <QSignalMapper>
#include <QString>
#include <QTimer>
#include <algorithm>
#include <iterator>
#include <map>

namespace popup_widget
{
namespace
{
/** Helper struct that prints custom frame on specified widget
 *
 */
struct Frame {
    explicit Frame(const int size, const int radius)
        : m_FrameSize(size)
        , m_radius(radius)
    {
        ;
    }

    ~Frame()
    {
        if (true == m_painter.isActive()) {
            m_painter.end();
        }
    }

    void operator()(QWidget *widget)
    {
        if (nullptr == widget) {
            return;
        }

        auto wWidth = [&]() { return widget->geometry().width(); };
        auto wHeight = [&]() { return widget->geometry().height(); };

        auto palette = QApplication::palette(widget);
        // trying to get the same colour as the border of QLineEdit widget. The call to highlight() of QPalette was
        // found empirically.
        m_pen.setColor(palette.highlight().color());
        m_pen.setWidth(m_FrameSize);

        m_painter.begin(widget);
        m_painter.setRenderHint(QPainter::Antialiasing, true);
        // set used layout's background
        m_painter.setBrush(palette.brush(QPalette::Active, QPalette::Base));
        m_painter.setPen(m_pen);

        if (m_FrameSize > 1) {
            m_FrameSize /= 2;
        }

        // start printing from bottom right side after rounded corner
        m_borderPath.moveTo(wWidth() - m_FrameSize - m_radius, wHeight() - m_FrameSize);
        //        m_borderPath.lineTo(m_FrameSize + m_radius, wHeight() - m_FrameSize);
        m_borderPath.moveTo(m_FrameSize /*+ m_radius*/, wHeight() - m_FrameSize);
        //        printBottomLeftCorner();

        // now print the left edge and left top corner
        m_borderPath.lineTo(m_FrameSize, m_FrameSize + m_radius);
        printTopLeftCorner();

        // now print the top edge and right top corner
        m_borderPath.lineTo(wWidth() - m_FrameSize - m_radius, m_FrameSize);
        printTopRightCorner();

        // now print the right edge and bottom right corner
        m_borderPath.lineTo(wWidth() - m_FrameSize, wHeight() - m_FrameSize /*- m_radius*/);
        //        printBottomRightCorner();

        m_painter.drawPath(m_borderPath);
        m_painter.end();
    }

private:
    qreal &currX() const { return m_borderPath.currentPosition().rx(); }

    qreal &currY() const { return m_borderPath.currentPosition().ry(); }

    // start printing from bottom side
    void printTopLeftCorner()
    {
        c1 = {currX(), currY() - m_radius};
        c2 = {currX() + m_radius, currY() - m_radius};
        m_borderPath.cubicTo(c1, c2, c2);
    }

    // start printing from left side
    void printTopRightCorner()
    {
        c1 = {currX() + m_radius, currY()};
        c2 = {currX() + m_radius, currY() + m_radius};
        m_borderPath.cubicTo(c1, c2, c2);
    }

    // start printing from top side
    void printBottomRightCorner()
    {
        c1 = {currX(), currY() + m_radius};
        c2 = {currX() - m_radius, currY() + m_radius};
        m_borderPath.cubicTo(c1, c2, c2);
    }

    // start printing from right side
    void printBottomLeftCorner()
    {
        c1 = {currX() - m_radius, currY()};
        c2 = {currX() - m_radius, currY() - m_radius};
        m_borderPath.cubicTo(c1, c2, c2);
    }

private:
    int m_FrameSize;
    QPen m_pen;
    QPainter m_painter;
    QPainterPath m_borderPath;
    qreal m_radius;
    QPointF c1;
    QPointF c2;
}; // end of struct

} // end of namespace

CtrlCharactersPopup::CtrlCharactersPopup(QLineEdit &parent)
    : QWidget(&parent)
    , m_lineEdit(parent)
    , m_hexMode(false)
{

    fillCtrlCharsContainer();

    // set size constraints
    const int minW = ((BUTTONS_IN_ROW + 1) * BUTTON_SIZE) + 100;
    setFixedWidth(minW);
    setFixedHeight(BUTTON_SIZE * 4);

    setFocusProxy(&m_lineEdit);

    setWindowFlags(Qt::FramelessWindowHint | Qt::Popup);

    // This widget and layout and childs (widgets) aloso has transparent background, disable border
    setAttribute(Qt::WA_TranslucentBackground, true);
    setAttribute(Qt::WA_ShowWithoutActivating, true);

    auto *holder = new QWidget(this);
    holder->setObjectName("holder");
    holder->setStyleSheet("QWidget#holder {background:transparent; border:0px solid white;}");
    holder->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    // Create grid layout to store buttons
    m_gridLayout = new QGridLayout(this);
    holder->setLayout(m_gridLayout);

    createControlButtons();

    // Create QScrollArea to store buttons and provide QScrollBar to manage relatively big number of buttons
    m_scrollArea = new QScrollArea(this);
    m_scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarPolicy::ScrollBarAlwaysOff);
    // Disable border and set transparent background for qscrollarea
    m_scrollArea->setObjectName("scrollArea");
    m_scrollArea->setStyleSheet("QScrollArea#scrollArea {background-color:transparent; border: none;}");
    m_scrollArea->setWidget(holder);

    QSizePolicy sizePolicy{QSizePolicy::Preferred, QSizePolicy::Preferred};
    sizePolicy.setHorizontalStretch(5);
    m_scrollArea->setSizePolicy(sizePolicy);

    // Create qlabel to store information about hovered / pressed button
    m_infoLabel = new QLabel(this);
    m_infoLabel->setText("");
    sizePolicy.setHorizontalStretch(0);
    m_infoLabel->setSizePolicy(sizePolicy);

    // Create horizontal layout to store QScrollArea with buttons and simple labels that
    // will show some information about hovered button
    auto *vl = new QHBoxLayout(this);
    vl->addWidget(m_scrollArea);
    vl->addWidget(m_infoLabel);
    this->setLayout(vl);

    m_activatingTimer.setSingleShot(true);
    connect(&m_activatingTimer, &QTimer::timeout, this,
            &popup_widget::CtrlCharactersPopup::timedShowTimerExpiredHandler);
}

void CtrlCharactersPopup::timedShow(const int ms)
{
    if (ms > 0) {
        if (true == m_activatingTimer.isActive()) {
            m_activatingTimer.stop();
        }
        m_activatingTimer.start(ms);
    }
}

void CtrlCharactersPopup::timedShowTimerExpiredHandler()
{
    // check if Ctrl key is pressed
    QFlags<Qt::KeyboardModifier> modifiers = QApplication::queryKeyboardModifiers();
    if (true == modifiers.testFlag(Qt::ControlModifier)) {
        // show and move window
        show();
    }
}

void CtrlCharactersPopup::buttonClickedSlot(int c)
{
    int curPos = m_lineEdit.cursorPosition();

    QString text = m_lineEdit.text();

    bool putAsQchar = true;

    // check if put as hex value or as just UTF-8 QChar
    if (true == m_hexMode) {
        do {
            putAsQchar = false;

            if (0 == curPos) {
                break;
            }

            // last occurrence of double quote on the left side
            int idx = text.lastIndexOf(QChar{'"'}, curPos - 1);

            if (-1 == idx) {
                break;
            }

            // last occurrence of double quote on the right side
            idx = text.indexOf(QChar{'"'}, curPos);
            if (-1 == idx) {
                break;
            }

            // cursor is between double quotes so put as QChar symbol
            putAsQchar = true;
        } while (0);
    }

    if (true == putAsQchar) {
        m_lineEdit.setText(text.insert(curPos, QChar{0x2400 + c}));
        // update cursor position inside the widget
        m_lineEdit.setCursorPosition(curPos + 1);
    } else {
        // put as hex value
        m_lineEdit.setText(text.insert(curPos, QString("%1").arg(c, 2, 16, QChar{'0'})));
        m_lineEdit.setCursorPosition(curPos + 2);
    }

    updateInfoLabel(c);

    // scroll to used element
    auto it = findElementInVector(c);
    if ((m_ctrlChars.end() != it) && (nullptr != std::get<2>(*it))) {
        m_scrollArea->ensureWidgetVisible(std::get<2>(*it), 5, 5);
    }
}

void CtrlCharactersPopup::buttonFocusInSlot(int c) { updateInfoLabel(c); }

void CtrlCharactersPopup::updateInfoLabel(const int c)
{
    // find element in the container
    auto it = findElementInVector(c);

    QString txt = QString{"Dec: %1\nHex: %2"}.arg(c).arg(c, 2, 16, QChar('0'));
    // append key sequence, nothing will be printed when QKeySequence(Qt::NoButtons) was assigned
    if (m_ctrlChars.end() != it) {
        txt.append(QString("\n%1").arg(std::get<1>(*it).toString()));
    }

    m_infoLabel->setText(txt);
}

void CtrlCharactersPopup::paintEvent(QPaintEvent *e)
{
    QWidget::paintEvent(e);

    // Paint custom border
    Frame printFrame{2, 5};
    printFrame(this);
}

void CtrlCharactersPopup::keyReleaseEvent(QKeyEvent *event)
{
    if (Qt::Key_Control == event->key()) {
        m_infoLabel->setText("");
        close();
    } else {
        QWidget::keyReleaseEvent(event);
    }
}

void CtrlCharactersPopup::show()
{
    QWidget::show();

    const QRect lineEditRect = m_lineEdit.geometry();
    const QPoint lineEditPos = lineEditRect.topLeft();
    const QRect popupRect = frameGeometry();

    int xPos = lineEditPos.x() + 1;
    // Don't try to position the popup widget while the width of text field is shorter than popup's width
    if ((popupRect.width() + 20) <= lineEditRect.width()) {
        // calculate distance to where the cursor is by measuring the string using font metrics
        QFontMetrics fm{m_lineEdit.font()};

        // get cursor position at the beginning of qtextedit -> point (coordinates are relative to the qlineedit) was
        // set empirically, because there is an some margin in qtextedit which value cannot be taken from API, note that
        // this can lead to position error when this margin will in much different size in other system.
        const int beginningTextCursPos = m_lineEdit.cursorPositionAt(QPoint{5, lineEditRect.height() / 2});
        const int currentTextCursPos = m_lineEdit.cursorPosition();
        QString tempStr = m_lineEdit.displayText().mid(beginningTextCursPos, currentTextCursPos - beginningTextCursPos);

        xPos = lineEditPos.x() + fm.width(tempStr) - (popupRect.width() / 2);
        if (xPos < lineEditPos.x() + 1) {
            // align to the left edge of text field
            xPos = lineEditPos.x() + 1;
        } else if (xPos > (lineEditPos.x() + m_lineEdit.width() - popupRect.width())) {
            // align to the right edge of text field
            xPos = lineEditPos.x() + m_lineEdit.width() - popupRect.width() - 1;
        }
    }

    QPoint widgetPos;
    widgetPos.setX(xPos);
    // move up but need to overlay some pixels that border of text field should be not visible
    widgetPos.setY(lineEditPos.y() - popupRect.height() + 4);
    move(m_lineEdit.parentWidget()->mapToGlobal(widgetPos));
}

void CtrlCharactersPopup::fillCtrlCharsContainer()
{
    // Append first most common used control characters and their shortcuts
    m_ctrlChars.insert(m_ctrlChars.begin(),
                       {
                           std::make_tuple(0x00, Qt::CTRL + Qt::Key_At, nullptr),
                           std::make_tuple(0x01, Qt::CTRL + Qt::Key_A, nullptr),
                           std::make_tuple(0x02, Qt::CTRL + Qt::Key_B, nullptr),
                           std::make_tuple(0x03, Qt::CTRL + Qt::Key_C, nullptr),
                           std::make_tuple(0x04, Qt::CTRL + Qt::Key_D, nullptr),
                           std::make_tuple(0x05, Qt::CTRL + Qt::Key_E, nullptr),
                           std::make_tuple(0x06, Qt::CTRL + Qt::Key_F, nullptr),
                           std::make_tuple(0x07, Qt::CTRL + Qt::Key_G, nullptr),
                           std::make_tuple(0x08, Qt::CTRL + Qt::Key_H, nullptr),
                           std::make_tuple(0x09, Qt::CTRL + Qt::Key_I, nullptr),
                           std::make_tuple(0x0A, Qt::CTRL + Qt::Key_J, nullptr),
                           std::make_tuple(0x0B, Qt::CTRL + Qt::Key_K, nullptr),
                           std::make_tuple(0x0C, Qt::CTRL + Qt::Key_L, nullptr),
                           std::make_tuple(0x0D, Qt::CTRL + Qt::Key_M, nullptr),
                           std::make_tuple(0x0E, Qt::CTRL + Qt::Key_N, nullptr),
                           std::make_tuple(0x0F, Qt::CTRL + Qt::Key_O, nullptr),
                           std::make_tuple(0x10, Qt::CTRL + Qt::Key_P, nullptr),
                           std::make_tuple(0x11, Qt::CTRL + Qt::Key_Q, nullptr),
                           std::make_tuple(0x12, Qt::CTRL + Qt::Key_R, nullptr),
                           std::make_tuple(0x13, Qt::CTRL + Qt::Key_S, nullptr),
                           std::make_tuple(0x14, Qt::CTRL + Qt::Key_T, nullptr),
                           std::make_tuple(0x15, Qt::CTRL + Qt::Key_U, nullptr),
                           std::make_tuple(0x16, Qt::CTRL + Qt::Key_V, nullptr),
                           std::make_tuple(0x17, Qt::CTRL + Qt::Key_W, nullptr),
                           std::make_tuple(0x18, Qt::CTRL + Qt::Key_X, nullptr),
                           std::make_tuple(0x19, Qt::CTRL + Qt::Key_Y, nullptr),
                           std::make_tuple(0x1A, Qt::CTRL + Qt::Key_Z, nullptr),
                           std::make_tuple(0x1B, Qt::CTRL + Qt::Key_BracketLeft, nullptr), // Ctrl+[
                           std::make_tuple(0x1C, Qt::CTRL + Qt::Key_Backslash, nullptr),
                           std::make_tuple(0x1D, Qt::CTRL + Qt::Key_BracketRight, nullptr), // Ctrl+]
                           std::make_tuple(0x1E, Qt::CTRL + Qt::Key_AsciiCircum, nullptr),  // Ctrl+^
                           std::make_tuple(0x1F, Qt::CTRL + Qt::Key_Underscore, nullptr)    // Ctrl+_
                       });
}

void CtrlCharactersPopup::createControlButtons()
{
    // Using custom font for buttons string to make visible utf8 chars
    QFont btnFont{"Helvetica", 19, QFont::Black};
    btnFont.setStyleStrategy(QFont::PreferAntialias);

    QSignalMapper *signalMapper = new QSignalMapper(this);

    // add buttons that represent control characters
    for (std::size_t i = 0; i < m_ctrlChars.size(); ++i) {
        auto *btn = new custom_ctrl_button::CustomQPushButton{QChar{0x2400 + std::get<0>(m_ctrlChars[i])}, this};
        btn->setFont(btnFont);
        btn->setFixedWidth(BUTTON_SIZE);
        btn->setFixedHeight(BUTTON_SIZE);
        // set shortcut for button
        if (std::get<1>(m_ctrlChars[i]) != QKeySequence{Qt::NoButton}) {
            btn->setShortcut(std::get<1>(m_ctrlChars[i]));
            // save btn address
            std::get<2>(m_ctrlChars[i]) = btn;
        }
        connect(btn, SIGNAL(clicked()), signalMapper, SLOT(map()));
        connect(btn, &custom_ctrl_button::CustomQPushButton::focusIn, this, &CtrlCharactersPopup::buttonFocusInSlot);
        signalMapper->setMapping(btn, std::get<0>(m_ctrlChars[i]));
        m_gridLayout->addWidget(btn, i / BUTTONS_IN_ROW, i % BUTTONS_IN_ROW, 1, 1);
    }

    connect(signalMapper, static_cast<void (QSignalMapper::*)(int)>(&QSignalMapper::mapped), this,
            &CtrlCharactersPopup::buttonClickedSlot);
}

void CtrlCharactersPopup::setHexInsertionMode(bool isHexMode) { m_hexMode = isHexMode; }

CtrlCharactersPopup::VectorConstIterator CtrlCharactersPopup::findElementInVector(const int c)
{
    return std::find_if(m_ctrlChars.begin(), m_ctrlChars.end(),
                        [c](decltype(*m_ctrlChars.end()) &item) -> bool { return c == std::get<0>(item); });
}

namespace custom_ctrl_button
{
CustomQPushButton::CustomQPushButton(QChar c, QWidget *parent)
    : QPushButton(c, parent)
    , m_assignedChar(c.unicode() & 0xFF)
{
    ;
}

void CustomQPushButton::focusInEvent(QFocusEvent *event)
{
    QPushButton::focusInEvent(event);

    if (true == event->gotFocus()) {
        emit focusIn(m_assignedChar);
    }
}

void CustomQPushButton::enterEvent(QEvent *event)
{
    QPushButton::enterEvent(event);
    emit focusIn(m_assignedChar);
}

} // end of namespace custom_ctrl_button

} // end of namespace popup_widget
