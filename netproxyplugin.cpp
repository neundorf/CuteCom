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

#include "netproxyplugin.h"
#include "ui_netproxyplugin.h"
#include <QtWidgets/QPushButton>

#define TRACE                                                                                                          \
    if (!debug) {                                                                                                      \
    } else                                                                                                             \
        qDebug()

static bool debug = false;

#define LED_GRAY QPixmap(QString::fromUtf8(":/images/led-circle-grey-md.png"))
#define LED_GREEN QPixmap(QString::fromUtf8(":/images/led-circle-green-md.png"))
#define LED_RED QPixmap(QString::fromUtf8(":/images/led-circle-red-md.png"))
#define LED_YELLOW QPixmap(QString::fromUtf8(":/images/led-circle-yellow-md.png"))

#define NET_ACT_LED(LED_ID, LED_UI_OBJ) new NetActLed(LED_ID, LED_UI_OBJ)

NetProxyPlugin::NetProxyPlugin(QFrame *parent, Settings *settings)
    : QFrame(parent)
    , m_settings(settings)
    , ui(new Ui::NetProxyPlugin)
{
    ui->setupUi(this);
    /* Plugin by default disabled, no injection, has QFrame, no injection process cmd */
    m_plugin = new Plugin(this, "NetProxy", this);

    m_proxySettings = new NetProxySettings(settings, this);
    /* event to show the macro dialog */
    connect(ui->m_bt_settings, &QPushButton::clicked, m_proxySettings, &NetProxySettings::show);
    /* unload */
    connect(ui->m_bt_unload, &QPushButton::clicked, this, &NetProxyPlugin::removePlugin);
    /* handle LEDs */
    connect(m_proxySettings, &NetProxySettings::ledSetValue, this, &NetProxyPlugin::ledSetValue);
    /* data from netproxy -> plugin manager*/
    connect(m_proxySettings, &NetProxySettings::sendCmd, this, &NetProxyPlugin::sendCmd);
    /* data from plugin manager -> netproxy */
    connect(this, &NetProxyPlugin::proxyCmd, m_proxySettings, &NetProxySettings::proxyCmd);
    /* connect status labels */
    connect(m_proxySettings, &NetProxySettings::udpStatus, this, &NetProxyPlugin::setUdpStatusText);
    connect(m_proxySettings, &NetProxySettings::tcpStatus, this, &NetProxyPlugin::setTcpStatusText);
    ui->m_lbl_udp_status->setText(tr("Not used"));
    ui->m_lbl_tcp_status->setText(tr("Not used"));

    /* setup leds */
    m_leds = new NetActLed *[NetProxySettings::en_led::NUMBER_OF_LEDS] {
        NET_ACT_LED(NetProxySettings::en_led::LED_UDP_EN, ui->m_led_udp_enable),
            NET_ACT_LED(NetProxySettings::en_led::LED_UDP_TX, ui->m_led_udp_tx),
            NET_ACT_LED(NetProxySettings::en_led::LED_UDP_RX, ui->m_led_udp_rx),
            NET_ACT_LED(NetProxySettings::en_led::LED_TCP_EN, ui->m_led_tcp_enable),
            NET_ACT_LED(NetProxySettings::en_led::LED_TCP_TX, ui->m_led_tcp_tx),
            NET_ACT_LED(NetProxySettings::en_led::LED_TCP_RX, ui->m_led_tcp_rx),
    };

    for (size_t i = NetProxySettings::en_led::LED_UDP_EN; i <= NetProxySettings::en_led::LED_TCP_RX; i++) {
        if ((m_leds[i]->m_index != NetProxySettings::en_led::LED_UDP_EN)
            && (m_leds[i]->m_index != NetProxySettings::en_led::LED_TCP_EN)) {
            m_leds[i]->m_tmr = new QTimer(m_leds[i]);
            connect(m_leds[i]->m_tmr, &QTimer::timeout, this, &NetProxyPlugin::tmrInterrupt);
        }
    }
    /* Checkbox enable event for UDP */
    connect(ui->m_cb_udp, &QCheckBox::clicked, this, [=]() {
        QCheckBox *cb = qobject_cast<QCheckBox *>(sender());
        if (cb->isChecked()) {
            m_proxySettings->bindUdp();
        } else {
            m_proxySettings->unbindUdp();
        }
    });
    /* Checkbox enable event for TCP */
    connect(ui->m_cb_tcp, &QCheckBox::clicked, this, [=]() {
        QCheckBox *cb = qobject_cast<QCheckBox *>(sender());
        if (cb->isChecked()) {
            m_proxySettings->startTcpServer();
        } else {
            m_proxySettings->stopTcpServer();
        }
    });
}

NetProxyPlugin::~NetProxyPlugin()
{
    TRACE << "[NetProxyPlugin] ~()";
    delete ui;
}

/**
 * @brief Return a pointer to the plugin data
 * @return
 */
const Plugin *NetProxyPlugin::plugin() { return m_plugin; }

/**
 * @brief [SLOT] Send unload command to the plugin manager
 */
void NetProxyPlugin::removePlugin(bool)
{
    TRACE << "[NetProxyPlugin::removePlugin]";
    emit unload(m_plugin);
}

/**
 * @brief Set LED values for UDP/TCP enable
 * @param led Which LED
 * @param value true/false
 */
void NetProxyPlugin::ledSetValue(NetProxySettings::en_led led, bool value)
{
    if (value)
        m_leds[led]->m_icon->setPixmap(LED_GREEN);
    else
        m_leds[led]->m_icon->setPixmap(LED_GRAY);
    TRACE << "[LED]NetProxyPlugin::ledSetValue] " << led << " set to " << value;
    if ((led != NetProxySettings::en_led::LED_UDP_EN) && (led != NetProxySettings::en_led::LED_TCP_EN)) {
        m_leds[led]->m_tmr->start(NetActLed::LED_INTERVAL_MS);
        TRACE << "[NetProxyPlugin::ledSetValue] timer started";
    }
}

/**
 * @brief Timer interrupt. The timers are used only for the LED
 *  effect for data Tx/Rx. Every time new data are in/out then
 *  a timer is triggered and this is the timeout ISR.
 */
void NetProxyPlugin::tmrInterrupt(void)
{
    QTimer *tmr = qobject_cast<QTimer *>(sender());
    NetActLed *led = qobject_cast<NetActLed *>(tmr->parent());

    led->m_icon->setPixmap(LED_GRAY);
}

/**
 * @brief [SLOT] This is only used to update the user interface for
 *  the UDP status. The signal is emitted from the settings form
 * @param state true: check checkbox m_cb_udp, false: uncheck
 * @param text The text to write in m_lbl_udp_status.
 */
void NetProxyPlugin::setUdpStatusText(bool state, QString text)
{
    if (state)
        ui->m_cb_udp->setCheckState(Qt::CheckState::Checked);
    else
        ui->m_cb_udp->setCheckState(Qt::CheckState::Unchecked);

    ui->m_lbl_udp_status->setText(text);
}

/**
 * @brief [SLOT] This is only used to update the user interface for
 *  the TCP status. The signal is emitted from the settings form
 * @param state true: check checkbox m_cb_tcp, false: uncheck
 * @param text The text to write in m_lbl_tcp_status.
 */
void NetProxyPlugin::setTcpStatusText(bool state, QString text)
{
    if (state)
        ui->m_cb_tcp->setCheckState(Qt::CheckState::Checked);
    else
        ui->m_cb_tcp->setCheckState(Qt::CheckState::Unchecked);

    ui->m_lbl_tcp_status->setText(text);
}
