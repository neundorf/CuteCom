/*
 * Copyright (c) 208 Dimitris Tassopoulos <dimtass@gmail.com>
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

#include "macrosettings.h"
#include <QDebug>
#include <QMessageBox>

#define MACRO_ITEM(CMD, NAME, TMR_INTERVAL, BUTTON, TMR_ACTIVE, TMR)                                                   \
    new macro_item(CMD, NAME, TMR_INTERVAL, BUTTON, TMR_ACTIVE, TMR)

MacroSettings::MacroSettings(QLineEdit *inputEdit, QWidget *parent)
    : QDialog(parent)
    , m_mainForm(parent)
    , m_inputEdit(inputEdit)
{
    setupUi(this);
    this->setWindowTitle("Macro Settings");
    /* Create an array of macro objects in order to make code simpler */
    m_macros = new macro_item *[NUM_OF_BUTTONS] {
        MACRO_ITEM(m_le_macro_txt_1, m_le_macro_bt_txt_1, m_sb_macro_tmr_1, m_bt_macro_1, m_cb_enable_macro_tmr_1,
                   new QTimer(this)),
            MACRO_ITEM(m_le_macro_txt_2, m_le_macro_bt_txt_2, m_sb_macro_tmr_2, m_bt_macro_2, m_cb_enable_macro_tmr_2,
                       new QTimer(this)),
            MACRO_ITEM(m_le_macro_txt_3, m_le_macro_bt_txt_3, m_sb_macro_tmr_3, m_bt_macro_3, m_cb_enable_macro_tmr_3,
                       new QTimer(this)),
            MACRO_ITEM(m_le_macro_txt_4, m_le_macro_bt_txt_4, m_sb_macro_tmr_4, m_bt_macro_4, m_cb_enable_macro_tmr_4,
                       new QTimer(this)),
            MACRO_ITEM(m_le_macro_txt_5, m_le_macro_bt_txt_5, m_sb_macro_tmr_5, m_bt_macro_5, m_cb_enable_macro_tmr_5,
                       new QTimer(this)),
            MACRO_ITEM(m_le_macro_txt_6, m_le_macro_bt_txt_6, m_sb_macro_tmr_6, m_bt_macro_6, m_cb_enable_macro_tmr_6,
                       new QTimer(this)),
            MACRO_ITEM(m_le_macro_txt_7, m_le_macro_bt_txt_7, m_sb_macro_tmr_7, m_bt_macro_7, m_cb_enable_macro_tmr_7,
                       new QTimer(this)),
            MACRO_ITEM(m_le_macro_txt_8, m_le_macro_bt_txt_8, m_sb_macro_tmr_8, m_bt_macro_8, m_cb_enable_macro_tmr_8,
                       new QTimer(this)),
            MACRO_ITEM(m_le_macro_txt_9, m_le_macro_bt_txt_9, m_sb_macro_tmr_9, m_bt_macro_9, m_cb_enable_macro_tmr_9,
                       new QTimer(this)),
            MACRO_ITEM(m_le_macro_txt_10, m_le_macro_bt_txt_10, m_sb_macro_tmr_10, m_bt_macro_10,
                       m_cb_enable_macro_tmr_10, new QTimer(this)),
            MACRO_ITEM(m_le_macro_txt_11, m_le_macro_bt_txt_11, m_sb_macro_tmr_11, m_bt_macro_11,
                       m_cb_enable_macro_tmr_11, new QTimer(this)),
            MACRO_ITEM(m_le_macro_txt_12, m_le_macro_bt_txt_12, m_sb_macro_tmr_12, m_bt_macro_12,
                       m_cb_enable_macro_tmr_12, new QTimer(this)),
            MACRO_ITEM(m_le_macro_txt_13, m_le_macro_bt_txt_13, m_sb_macro_tmr_13, m_bt_macro_13,
                       m_cb_enable_macro_tmr_13, new QTimer(this)),
            MACRO_ITEM(m_le_macro_txt_14, m_le_macro_bt_txt_14, m_sb_macro_tmr_14, m_bt_macro_14,
                       m_cb_enable_macro_tmr_14, new QTimer(this)),
            MACRO_ITEM(m_le_macro_txt_15, m_le_macro_bt_txt_15, m_sb_macro_tmr_15, m_bt_macro_15,
                       m_cb_enable_macro_tmr_15, new QTimer(this)),
            MACRO_ITEM(m_le_macro_txt_16, m_le_macro_bt_txt_16, m_sb_macro_tmr_16, m_bt_macro_16,
                       m_cb_enable_macro_tmr_16, new QTimer(this)),
    };

    /* Setup signal/slots */
    for (size_t i = 0; i < NUM_OF_BUTTONS; i++) {
        connect(m_macros[i]->button, &QPushButton::clicked, this, &MacroSettings::macroPress);
        connect(m_macros[i]->name, &QLineEdit::textChanged, this,
                [=]() { m_macros[i]->button->setText(m_macros[i]->name->text()); });
        connect(m_macros[i]->tmr_active, &QCheckBox::stateChanged, [=](int state) {
            if (state == Qt::Checked) {
                m_macros[i]->tmr->stop();
                m_macros[i]->tmr->setInterval(m_macros[i]->tmr_interval->text().toInt());
                m_macros[i]->tmr->start();
                qDebug() << "Timer " << i << " started.";
            } else {
                m_macros[i]->tmr->stop();
                qDebug() << "Timer " << i << " stopped.";
            }
        });
        connect(m_macros[i]->tmr, &QTimer::timeout, [=]() {
            m_inputEdit->setText(m_macros[i]->cmd->text());
            emit sendCmd();
        });
    }
    connect(m_bt_load_macros, &QPushButton::clicked, this, &MacroSettings::openFile);
    connect(m_bt_save_macros, &QPushButton::clicked, this, &MacroSettings::saveFile);

    m_lbl_macros_path->setText("");
}

/**
 * @brief Extract the button index. That's a bit dirty way to get indexes
 * from the buttons. For this to happen the buttons must always be named
 * as 'm_bt_macro_' and then the index.
 * @param btnName The button object name
 * @return int The index of the button that corresponds to m_macros[]
 */
int MacroSettings::getButtonIndex(QString btnName)
{
    int idx = -1;

    if (btnName.startsWith("m_bt_macro_")) {
        idx = btnName.remove("m_bt_macro_").toInt() - 1;
    }
    return idx;
}

/**
 * @brief Sends the cmd that corresponds to the macro button
 */
void MacroSettings::macroPress()
{
    QPushButton *button = qobject_cast<QPushButton *>(sender());
    if (button) {
        int idx = getButtonIndex(button->objectName());
        if (idx < 0)
            return;

        qDebug() << "Sender: " << m_macros[idx]->button->objectName();
        /* Get macro text */
        QString cmd = m_macros[idx]->cmd->text();
        m_inputEdit->setText(cmd);
        emit sendCmd();
    }
}

/**
 * @brief Load a macro file that is compatible with Br@y's terminal
 * @param fname The filename of the *.tmf file.
 */
void MacroSettings::loadFile(QString fname)
{
    if (fname.isEmpty())
        return;

    QFile file(fname);

    if (!file.open(QFile::ReadOnly | QFile::Text)) {
        QMessageBox::warning(this, "Could not open file", file.errorString());
        return;
    }
    QTextStream in(&file);
    if (parseFile(in))
        m_lbl_macros_path->setText(fname);

    file.close();
    if (QString::compare(m_macroFilename, fname, Qt::CaseSensitive)) {
        m_macroFilename = fname;
        m_lbl_macros_path->setText(m_macroFilename);
        emit fileChanged(m_macroFilename);
    }
}

void MacroSettings::openFile()
{
    QString fname = QFileDialog::getOpenFileName(this, "Open a bray's terminal macro settings file", QDir::homePath(),
                                                 "Macros (*.tmf)");
    if (!fname.isEmpty())
        loadFile(fname);
}

/**
 * @brief Parse a macro *.tmf file and fill the dialog form
 * @param in The file text stream
 * @return true if all gone OK, otherwise false
 */
bool MacroSettings::parseFile(QTextStream &in)
{
    /* Read the first line */
    QString line = in.readLine();
    if (line.startsWith("# Terminal macro file v2")) {
        for (size_t i = 0; i < NUM_OF_BUTTONS; i++) {
            /* Get button name */
            line = in.readLine();
            m_macros[i]->button->setText(line);
            m_macros[i]->name->setText(line);
            /* Get button cmd */
            line = in.readLine();
            m_macros[i]->cmd->setText(line);
        }
    } else {
        QMessageBox::warning(this, "Error", "Unsupported macro file!");
        return false;
    }
    return true;
}

void MacroSettings::saveFile()
{
    QDir dir(QDir::homePath());
    QString fname = QFileDialog::getSaveFileName(this, "Open a bray's terminal macro settings file",
                                                 dir.filePath(m_macroFilename), "Macros (*.tmf)");
    if (fname.isEmpty())
        return;

    qDebug() << "Save to: " << fname;
    QFile file(fname);
    if (!file.open(QFile::WriteOnly | QFile::Text)) {
        QMessageBox::warning(this, "Could not open file", file.errorString());
        return;
    }
    QTextStream out(&file);
    out << "# Terminal macro file v2" << endl;
    for (size_t i = 0; i < NUM_OF_BUTTONS; i++) {
        out << m_macros[i]->button->text() << endl;
        out << m_macros[i]->cmd->text() << endl;
    }
    out.flush();
    file.close();
    if (QString::compare(m_macroFilename, fname, Qt::CaseSensitive)) {
        m_macroFilename = fname;
        m_lbl_macros_path->setText(m_macroFilename);
        emit fileChanged(m_macroFilename);
    }
}
