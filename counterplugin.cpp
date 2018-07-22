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

#include "counterplugin.h"
#include "ui_counterplugin.h"
#include <QMessageBox>

#define TRACE                                                                                                          \
    if (!debug) {                                                                                                      \
    } else                                                                                                             \
        qDebug()

static bool debug = false;
static CounterPlugin *m_counter = NULL;
static int processCmd(const QString *text, QString *new_text);

CounterPlugin::CounterPlugin(QFrame *parent, Settings *settings)
    : QFrame(parent)
    , ui(new Ui::CounterPlugin)
    , m_settings(settings)
{
    ui->setupUi(this);
    /* Plugin by default disabled, no injection, has QFrame, no injection process cmd */
    m_plugin = new Plugin(this, "Byte Counter", this, (Plugin::processCmd_fp)&processCmd);
    /* reset values */
    ui->m_lbl_rx_value->setText("0");
    ui->m_lbl_tx_value->setText("0");
    ui->m_lbl_mrx_value->setText("0");
    ui->m_lbl_mtx_value->setText("0");

    connect(ui->m_bt_unload, &QPushButton::clicked, this, &CounterPlugin::removePlugin);
    connect(ui->m_bt_help, &QPushButton::clicked, this, &CounterPlugin::helpMsg);
    connect(ui->m_bt_clear, &QPushButton::clicked, this, [=]() {
        ui->m_lbl_rx_value->setText("0");
        ui->m_lbl_tx_value->setText("0");
    });
    connect(ui->m_bt_clear_memory, &QPushButton::clicked, this, [=]() {
        ui->m_lbl_mrx_value->setText("0");
        ui->m_lbl_mtx_value->setText("0");
    });
    connect(ui->m_bt_memory, &QPushButton::clicked, this, [=]() {
        ui->m_lbl_mrx_value->setText(ui->m_lbl_rx_value->text());
        ui->m_lbl_mtx_value->setText(ui->m_lbl_tx_value->text());
    });
    m_counter = this;

    TRACE << "[CounterPlugin::CounterPlugin]";
}

CounterPlugin::~CounterPlugin()
{
    delete ui;
    if (m_counter)
        m_counter = NULL;
}

/**
 * @brief Static function to use as proxy to call public
 *          member funtions from this object
 * @param text The data that are about to be sent
 * @param new_text The new data
 * @return int Always retutn 0 in this case
 */
int processCmd(const QString *text, QString *new_text)
{
    m_counter->txBytes(text->length());

    TRACE << "[CounterPlugin::processCmd] " << text->toLatin1();
    return 0;
}

/**
 * @brief Handle Tx bytes.
 * @param len The number of received bytes
 */
void CounterPlugin::txBytes(int len)
{
    int value = m_counter->ui->m_lbl_tx_value->text().toInt();
    value += len;
    ui->m_lbl_tx_value->setText(QString::number(value));
}

/**
 * @brief
 * @param data
 */
void CounterPlugin::rxBytes(QByteArray data)
{
    TRACE << "[CounterPlugin::rxBytes]: " << data.length();
    int value = ui->m_lbl_rx_value->text().toInt();
    value += data.length();
    ui->m_lbl_rx_value->setText(QString::number(value));
}

/**
 * @brief Return a pointer to the plugin data
 * @return
 */
const Plugin *CounterPlugin::plugin() { return m_plugin; }

/**
 * @brief [SLOT] Send unload command to the plugin manager
 */
void CounterPlugin::removePlugin(bool) { emit unload(m_plugin); }

/**
 * @brief Help message for the Byte counter plugin
 */
void CounterPlugin::helpMsg(void)
{
    QString help_str = tr("This plugin provides a TX/RX byte counting functionality.\n"
                          "This is useful in cases that you need to either count the\n"
                          "bytes of a transaction or verify that the correct number of\n"
                          "bytes has beed send or received.\n\n"
                          "The plugin also supports to save the current Tx/Rx values to\n"
                          "the MTx/MRx. Also, you can clear those values independently.\n\n"
                          "Press 'C' to clear the Tx/Rx values\n"
                          "Press 'M' to store the Tx/Rx to MTx/MRx\n"
                          "Press 'CM' to clear the MTx/Rx values\n");

    QMessageBox::information(this, tr("How to use TCP forwarding"), help_str);
}
