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

#include "controlpanel.h"

#include <QSerialPortInfo>
#include <QtWidgets/QComboBox>
#include <QLineEdit>
#include <QIntValidator>
#include <QPropertyAnimation>
#include <QFileDialog>

#include "qdebug.h"
//#include <functional>

ControlPanel::ControlPanel(QWidget *parent, Settings *settings)
    : QFrame(parent)
    , m_x(3)
{
    this->setupUi(this);

    m_baudValidator = new QIntValidator(0, 999000, this);
    m_combo_Baud->setInsertPolicy(QComboBox::NoInsert);
    const Settings::Session session = settings->getCurrentSession();

    fillDeviceCombo(session.device);
    fillBaudCombo();
    fillDataBitCombo();
    fillStopBitCombo();
    fillParityCombo();
    fillFlowCombo();
    fillOpenModeCombo();
    m_check_lineBreak->setChecked(session.showCtrlCharacters);
    m_check_timestamp->setChecked(session.showTimestamp);

    connect(m_check_lineBreak, &QCheckBox::toggled,
            [=](bool checked) { emit settingChanged(Settings::ShowCtrlCharacters, checked); });
    connect(m_check_timestamp, &QCheckBox::toggled,
            [=](bool checked) { emit settingChanged(Settings::ShowTimestamp, checked); });
    connect(this, &ControlPanel::settingChanged, settings, &Settings::settingChanged);

    applySessionSettings(session);

    connect(m_bt_logfileDialog, &QToolButton::clicked, this, &ControlPanel::chooseLogFile);

    setAutoFillBackground(true);
    //    setWindowOpacity(0.1);
    m_bt_open->setCheckable(true);
    // Disable RTS and CTS checkboxes during construction
    m_rts_line->setEnabled(false);
    m_dtr_line->setEnabled(false);

    // Connect button signal to slot
    connect(m_bt_settings, &QPushButton::clicked, this, &ControlPanel::toggleMenu);
    connect(m_bt_open, &QPushButton::clicked, this, &ControlPanel::toggleDevice);
    connect(m_combo_Baud, static_cast<void (QComboBox::*)(int)>(&QComboBox::activated), this,
            &ControlPanel::customBaudRate);
}

/**
 *
 * @brief ControlPanel::collapse
 */
void ControlPanel::collapse()
{
    QPoint btnPosition = m_bt_settings->mapToParent(m_bt_settings->rect().topLeft());

    m_y = -(btnPosition.y() + 5);
    //    qDebug() << Q_FUNC_INFO << m_y << " : " << m_x;
    move(m_x, m_y);
    m_menuVisible = false;
}

/**
 * Makes the complete panel visible
 * @brief ControlPanel::slideOut
 */
void ControlPanel::slideOut()
{
    if (!m_menuVisible)
        toggleMenu();
}

ControlPanel::~ControlPanel() {}

// for debugging
void ControlPanel::printPosition()
{
    qDebug() << "toParent pos" << m_bt_settings->mapToParent(m_bt_settings->pos());
    qDebug() << "toParten topRight" << m_bt_settings->mapToParent(m_bt_settings->rect().topLeft());
}

void ControlPanel::toggleMenu()
{
    // Create animation
    QPropertyAnimation *animation = new QPropertyAnimation(this, "pos");
    // bool m_menuVisible = (y() < -3);
    QPoint endPos = m_menuVisible ? QPoint(m_x, m_y) : QPoint(m_x, -3);
    //    qDebug() << m_menuVisible << endPos;
    animation->setStartValue(pos());
    animation->setEndValue(endPos);
    animation->start();
    if (m_menuVisible) {
        m_bt_settings->setText("&Settings");
        m_menuVisible = false;
    } else {
        m_bt_settings->setText("^");
        m_bt_settings->setShortcut(Qt::KeyboardModifier::AltModifier + Qt::Key_S);
        m_menuVisible = true;
        m_combo_Baud->setFocus();
    }
}

void ControlPanel::toggleDevice(bool open)
{
    if (open) {
        if (m_menuVisible)
            toggleMenu();
        // Enable RTS and DTR checkboxes when opening device. Note that visibility of these
        // widgets depends on currently selected type of flow control.
        m_rts_line->setEnabled(true);
        m_dtr_line->setEnabled(true);

        m_bt_settings->setEnabled(false);
        m_bt_open->setText(tr("Cl&ose"));
        emit openDeviceClicked();
    } else {
        // Disable RTS and DTR checkboxes when closing device.
        m_rts_line->setEnabled(false);
        m_dtr_line->setEnabled(false);

        m_bt_settings->setEnabled(true);
        emit closeDeviceClicked();
        m_bt_open->setText(tr("&Open"));
    }
}

void ControlPanel::customBaudRate(int index)
{
    Q_UNUSED(index);
    if (m_combo_Baud->currentData() == -1) {
        m_combo_Baud->setEditable(true);

        m_baud_edit = m_combo_Baud->lineEdit();
        m_baud_edit->setValidator(m_baudValidator);
        m_baud_edit->selectAll();
        connect(m_baud_edit, &QLineEdit::editingFinished, this, &ControlPanel::customBaudRateSet);
    }
}

void ControlPanel::customBaudRateSet()
{
    m_combo_Baud->setEditable(false);
    m_combo_Baud->setItemText(m_combo_Baud->currentIndex(), m_baud_edit->text());
    settingChanged(Settings::BaudRate, m_baud_edit->text().toInt());
}

void ControlPanel::applySessionSettings(Settings::Session session)
{
    int index = m_combo_Baud->findText(QString::number(session.baudRate));
    int current_index = 0;
    if (index != -1) {
        m_combo_Baud->setCurrentIndex(index);
    } else {
        index = m_combo_Baud->findData(-1);
        if (index != -1) {
            m_combo_Baud->setItemText(index, QString::number(session.baudRate));
            m_combo_Baud->setCurrentIndex(index);
        }
    }

    index = m_combo_dataBits->findData(session.dataBits);
    if (index != -1) {
        m_combo_dataBits->setCurrentIndex(index);
    } else {
        current_index = m_combo_dataBits->currentIndex();
        index = m_combo_dataBits->findData(QSerialPort::Data8);
        if (index != -1 && index != current_index)
            m_combo_dataBits->setCurrentIndex(index);
        else
            emit settingChanged(Settings::DataBits, m_combo_dataBits->currentData());
    }

    index = m_combo_parity->findData(session.parity);
    if (index != -1) {
        m_combo_parity->setCurrentIndex(index);
    } else {
        current_index = m_combo_parity->currentIndex();
        index = m_combo_parity->findData(QSerialPort::NoParity);
        if (index != -1 && index != current_index)
            m_combo_parity->setCurrentIndex(index);
        else
            emit settingChanged(Settings::Parity, m_combo_parity->currentData());
    }

    index = m_combo_stopBits->findData(session.stopBits);
    if (index != -1) {
        m_combo_stopBits->setCurrentIndex(index);
    } else {
        current_index = m_combo_stopBits->currentIndex();
        int index = m_combo_stopBits->findData(QSerialPort::OneStop);
        if (index != -1 && index != current_index)
            m_combo_stopBits->setCurrentIndex(index);
        else
            emit settingChanged(Settings::StopBits, m_combo_stopBits->currentData());
    }

    index = m_combo_flowControl->findData(session.flowControl);
    if (index != -1) {
        m_combo_flowControl->setCurrentIndex(index);
    } else {
        current_index = m_combo_flowControl->currentIndex();
        index = m_combo_flowControl->findData(QSerialPort::NoFlowControl);
        if (index != -1 && index != current_index)
            m_combo_flowControl->setCurrentIndex(index);
        else
            emit settingChanged(Settings::FlowControl, m_combo_flowControl->currentData());
    }

    index = m_combo_Mode->findData(session.openMode);
    if (index != -1) {
        m_combo_Mode->setCurrentIndex(index);
    } else {
        current_index = m_combo_Mode->currentIndex();
        index = m_combo_Mode->findData(QIODevice::ReadWrite);
        if (index != -1 && index != current_index)
            m_combo_Mode->setCurrentIndex(index);
        else
            emit settingChanged(Settings::OpenMode, m_combo_Mode->currentData());
    }

    m_check_lineBreak->setChecked(session.showCtrlCharacters);
    m_check_timestamp->setChecked(session.showTimestamp);
}

void ControlPanel::fillDeviceCombo(const QString &deviceName)
{
    m_combo_device->clear();
    int numberDevices = 0;
    for (auto info : QSerialPortInfo::availablePorts()) {
        m_combo_device->addItem(info.systemLocation());
        if (info.isValid()) {
            QString deviceInfo = QString("%1 %2\n%3:%4 "
#if QT_VERSION < QT_VERSION_CHECK(5, 3, 0)
                                         )
#else
                                         "# %5")
#endif
                                     .arg(info.manufacturer())
                                     .arg(info.description())
                                     .arg(info.vendorIdentifier())
                                     .arg(info.productIdentifier())
#if QT_VERSION < QT_VERSION_CHECK(5, 3, 0)
                ;
#else
                                     .arg(info.serialNumber());
#endif
            m_combo_device->setItemData(numberDevices, deviceInfo, Qt::ToolTipRole);
            QVariant temp = m_combo_device->itemData(numberDevices, Qt::ToolTipRole);
        } else {
            m_combo_device->setItemData(numberDevices, tr("Not a valid device"), Qt::ToolTipRole);
        }
        numberDevices++;
    }
    connect(m_combo_device, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), [=](int index) {
        emit settingChanged(Settings::Device, m_combo_device->currentText());
        // unfortunately, this is not working - itemDate returns an invalid QVariant
        m_combo_device->setToolTip(m_combo_device->itemData(index, Qt::ToolTipRole).toString());
    });

    if (!deviceName.isEmpty()) {
        if (numberDevices) {
            int index = m_combo_device->findText(deviceName);
            if (index != -1) {
                // found something - work done!
                m_combo_device->setCurrentIndex(index);
                return;
            }
        }
        // maybe it's not connected at the moment
        // let's add it anyway to the list of devices since the
        // user might wan't to reconnect it
        m_combo_device->addItem(deviceName);
        m_combo_device->setCurrentIndex(numberDevices);
    } else {
        // use the first available port as default
        m_combo_Baud->setCurrentIndex(0);
    }
}

void ControlPanel::fillBaudCombo()
{
    m_combo_Baud->addItem(QStringLiteral("1200"), QSerialPort::Baud1200);
    m_combo_Baud->addItem(QStringLiteral("4800"), QSerialPort::Baud4800);
    m_combo_Baud->addItem(QStringLiteral("9600"), QSerialPort::Baud9600);
    m_combo_Baud->addItem(QStringLiteral("19200"), QSerialPort::Baud19200);
    m_combo_Baud->addItem(QStringLiteral("38400"), QSerialPort::Baud38400);
    m_combo_Baud->addItem(QStringLiteral("57600"), QSerialPort::Baud57600);
    m_combo_Baud->addItem(QStringLiteral("115200"), QSerialPort::Baud115200);
    m_combo_Baud->insertSeparator(90); // append this separator at the end
    m_combo_Baud->addItem(QStringLiteral("Custom"), -1);

    connect(m_combo_Baud, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), [=]() {
        int custom = m_combo_Baud->currentData().toInt();
        emit settingChanged(Settings::BaudRate,
                            (custom == -1) ? m_combo_Baud->currentText() : m_combo_Baud->currentData());
    });
}

void ControlPanel::fillFlowCombo()
{
    m_combo_flowControl->addItem(QStringLiteral("None"), QSerialPort::NoFlowControl);
    m_combo_flowControl->addItem(QStringLiteral("Hardware"), QSerialPort::HardwareControl);
    m_combo_flowControl->addItem(QStringLiteral("Software"), QSerialPort::SoftwareControl);

    connect(m_combo_flowControl, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
            [=]() { emit settingChanged(Settings::FlowControl, m_combo_flowControl->currentData()); });

    // Connect flow-control's combobox index changed signal to the slot that sets the visibility of RTS and DTR
    // checkboxes
    connect(m_combo_flowControl, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this,
            &ControlPanel::changeVisibilityOfRTSandDTRCheckboxes);
}

void ControlPanel::fillParityCombo()
{
    m_combo_parity->addItem(QStringLiteral("None"), QSerialPort::NoParity);
    m_combo_parity->addItem(QStringLiteral("Even"), QSerialPort::EvenParity);
    m_combo_parity->addItem(QStringLiteral("Odd"), QSerialPort::OddParity);
    m_combo_parity->addItem(QStringLiteral("Space"), QSerialPort::SpaceParity);
    m_combo_parity->addItem(QStringLiteral("Mark"), QSerialPort::MarkParity);

    connect(m_combo_parity, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
            [=]() { emit settingChanged(Settings::Parity, m_combo_parity->currentData()); });
}

void ControlPanel::fillDataBitCombo()
{
    m_combo_dataBits->addItem(QStringLiteral("5"), QSerialPort::Data5);
    m_combo_dataBits->addItem(QStringLiteral("6"), QSerialPort::Data6);
    m_combo_dataBits->addItem(QStringLiteral("7"), QSerialPort::Data7);
    m_combo_dataBits->addItem(QStringLiteral("8"), QSerialPort::Data8);

    connect(m_combo_dataBits, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
            [=]() { emit settingChanged(Settings::DataBits, m_combo_dataBits->currentData()); });
}

void ControlPanel::fillStopBitCombo()
{
    m_combo_stopBits->addItem(QStringLiteral("1"), QSerialPort::OneStop);
#ifdef Q_WS_WIN
    m_combo_stopBits->addItem(QStringLiteral("1.5"), QSerialPort::OneAndHalfStop);
#endif
    m_combo_stopBits->addItem(QStringLiteral("2"), QSerialPort::TwoStop);

    connect(m_combo_stopBits, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
            [=]() { emit settingChanged(Settings::StopBits, m_combo_stopBits->currentData()); });
}

void ControlPanel::fillOpenModeCombo()
{
    m_combo_Mode->addItem(tr("Read Only"), QIODevice::ReadOnly);
    m_combo_Mode->addItem(tr("Write Only"), QIODevice::WriteOnly);
    m_combo_Mode->addItem(tr("Read/Write"), QIODevice::ReadWrite);

    connect(m_combo_Mode, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
            [=]() { emit settingChanged(Settings::OpenMode, m_combo_Mode->currentData()); });
}

/**
 * Display a file chooser dialog letting the user choose
 * a file to log the output to
 * @brief ControlPanel::chooseLogFile
 */
void ControlPanel::chooseLogFile()
{
    QString logFile = QFileDialog::getSaveFileName(this, tr("Save log file ..."), m_logfile_edit->text());
    if (!logFile.isEmpty()) {
        m_logfile_edit->setText(logFile);
    }
}

void ControlPanel::changeVisibilityOfRTSandDTRCheckboxes(int index)
{
    // if index is valid then retrieve the flow control type and change visibility of RTS and DTR lines. RTS and CTS
    // checkboxes are invisible only if hardware control is enabled.
    if (-1 != index) {
        auto data = static_cast<QSerialPort::FlowControl>(m_combo_flowControl->itemData(index).toInt());
        if (QSerialPort::FlowControl::HardwareControl == data) {
            m_rts_line->setVisible(false);
            m_dtr_line->setVisible(false);
        } else {
            m_rts_line->setVisible(true);
            m_dtr_line->setVisible(true);
        }
    }
}
