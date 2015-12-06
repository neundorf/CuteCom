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

#ifndef CONTROLPANEL_H
#define CONTROLPANEL_H

#include "ui_controlpanel.h"
#include "settings.h"

#include <QFrame>
#include <QSerialPort>

class ControlPanel : public QFrame, public Ui::ControlPanel
{
    Q_OBJECT

signals:
    void openDeviceClicked();
    void closeDeviceClicked();
    void settingChanged(Settings::Options option, QVariant setting);

public:
    explicit ControlPanel(QWidget *parent = 0, Settings *settings = 0);
    ~ControlPanel();
    /**
     * Returns the space in the y-achsis needed
     * when the panel is collapsed
     * @brief hiddenHeight
     * @return
     */
    int hiddenHeight() { return height() + m_y; }
    void printPosition();
    void collapse();
    void slideOut();
    /**
     * @brief setLeftMargin
     * @param x
     */
    void setLeftMargin(int x) { m_x = x; }

    void fillDeviceCombo(const QString &deviceName);
    void applySessionSettings(Settings::Session settings);

    void closeDevice()
    {
        toggleDevice(false);
        m_bt_open->setChecked(false);
    }

private:
    void fillBaudCombo();
    void fillFlowCombo();
    void fillParityCombo();
    void fillDataBitCombo();
    void fillStopBitCombo();
    void fillOpenModeCombo();

    // slots
    void toggleMenu();
    void toggleDevice(bool open);
    void customBaudRate(int index);
    void customBaudRateSet();
    void chooseLogFile();

private:
    int m_x;
    int m_y;
    bool m_menuVisible;
    QIntValidator *m_baudValidator;
    QLineEdit *m_baud_edit;
};

#endif // CONTROLPANEL_H
