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

#ifndef CTRLCHARACTERSPOPUP_H
#define CTRLCHARACTERSPOPUP_H

#include <QGridLayout>
#include <QKeySequence>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QScrollArea>
#include <QTimer>
#include <QWidget>
#include <map>
#include <vector>

namespace popup_widget
{

/**
 * @brief This class act as popup window glued to the QLineEdit field and displays buttons that represents non-printable
 * control characters e.g. \n, \t. It has custom layout so opening it looks like it is part of the QLineEdit. User can
 * select given button by clicking on it or by pressing right shortcut (Ctrl + key) - not all buttons have  * assigned
 * shortcuts (these will have object: QKeySequence{Qt::NoButton}). After user clicked or used shortcut, then control
 * character in printable version (UTF-8 with offset 0x2400) is pasted into QLineEdit at current cursor position.
*/
class CtrlCharactersPopup : public QWidget
{
    Q_OBJECT

public:
    /**
     * @param parent QLineEdit widget on which popup window will be glued to and where printable
     * versions of control characters will be pasted to.
     */
    explicit CtrlCharactersPopup(QLineEdit &parent);
    virtual ~CtrlCharactersPopup() { ; }

    /**
     *  @brief Calling this method starts one shot timer with interval specified in input parameter then
     *  when Ctrl key is pressed then popup will show and will be visible until Ctrl key is released.
     *  @param ms interval in milliseconds for one shot timer.
     */
    void timedShow(const int ms);

    /**
     * @brief If set to true then during insertion of control character into qlineedit will check if cursor position
     * is between double quotes, if yes then will put there just UTF-8 printable character, if no then will put hex
     * value of given control character.
     * @param isHexMode true if current insertion mode is set to hex mode.
     */
    void setHexInsertionMode(bool isHexMode);

protected slots:

    /// @brief Called by one shot timer started by timedShow() method.
    void timedShowTimerExpiredHandler();

    /**
     * @brief Called when user points mouse cursor on buttons.
     * @param c value of control character connected to pointed button.
     */
    void buttonFocusInSlot(int c);

    /**
     * @brief Called when user clicks on button or uses shortcut connected with that button.
     * @param c value of control character connected to clicked button.
     */
    void buttonClickedSlot(int c);

protected:
    using TupleType = std::tuple<int, QKeySequence, QPushButton *>;
    using VectorConstIterator = std::vector<TupleType>::const_iterator;

    /// @brief Overrides to support custom border.
    void paintEvent(QPaintEvent *e) override;

    /// @brief Overrides to support closing this widget when user releases Ctrl key
    void keyReleaseEvent(QKeyEvent *event) override;

    void show();

    /// @brief Creates buttons which represent control characters.
    void createControlButtons();

    /// @brief Creates container with control characters and assigned shortcuts.
    void fillCtrlCharsContainer();
    void updateInfoLabel(const int c);
    /// @brief Searches container of control characters and returns const iterator to tuple which has the same value as
    /// input parameter: c at 0 position.
    VectorConstIterator findElementInVector(const int c);

protected:
    /// @brief Size of button which represent control character.
    static constexpr const int BUTTON_SIZE = 30;

    /// @brief Specifies how many buttons in the row will be placed.
    static constexpr const int BUTTONS_IN_ROW = 5;

    /// @brief QLineEdit widget to which this popup window must be glued to.
    QLineEdit &m_lineEdit;
    QGridLayout *m_gridLayout;
    QScrollArea *m_scrollArea;

    /// @brief Label which will show information about pointed button.
    QLabel *m_infoLabel;
    /// @brief This field informs if current insertion mode is set to hex
    bool m_hexMode;

    /// @brief Container that will hold control character value, assigned shortcut and pointer to CustomQPushButton
    /// which
    /// will be used to perform auto-scrolling.
    std::vector<TupleType> m_ctrlChars;
    /// @brief One shot timer using to delayed activation (showing) this widget.
    QTimer m_activatingTimer;
};

namespace custom_ctrl_button
{

/// @brief Custom PushButton that overrides focusInEvent and enterEvent.
class CustomQPushButton : public QPushButton
{
    Q_OBJECT

public:
    /**
     * @brief Constructs custom button.
     * @param c assigned control character in printable version (UTF-8 with 0x2400 offset).
     * @param parent parent widget.
     */
    explicit CustomQPushButton(QChar c, QWidget *parent = nullptr);
    virtual ~CustomQPushButton() { ; }

signals:
    /// @brief Emitted when user hovers button of if it has been activated by arrow keys.
    void focusIn(int c);

protected:
    /// @brief Overrides to emit focusIn() on focusInEvent().
    void focusInEvent(QFocusEvent *event) override;
    /// @brief Overrides to emit focusIn() on enterEvent().
    void enterEvent(QEvent *event) override;

private:
    /// @brief Stores assigned control character.
    int m_assignedChar;
}; // end of CustomQPushButton

} // end of namespace custom_ctrl_button

} // end of namespace popup_widget
#endif // CTRLCHARACTERSPOPUP_H
