#include "netproxyplugin.h"
#include "ui_netproxyplugin.h"
#include <QtWidgets/QPushButton>

#define TRACE if (!debug) {} else qDebug()
static bool debug = false;

#define LED_GRAY    QPixmap(QString::fromUtf8(":/images/led-circle-grey-md.png"))
#define LED_GREEN   QPixmap(QString::fromUtf8(":/images/led-circle-green-md.png"))
#define LED_RED     QPixmap(QString::fromUtf8(":/images/led-circle-red-md.png"))
#define LED_YELLOW  QPixmap(QString::fromUtf8(":/images/led-circle-yellow-md.png"))

#define NET_ACT_LED(LED_ID, LED_UI_OBJ)                                                   \
    new NetActLed(LED_ID, LED_UI_OBJ)

NetProxyPlugin::NetProxyPlugin(QFrame *parent, Settings * settings) :
    QFrame(parent),
    m_settings(settings),
    ui(new Ui::NetProxyPlugin)
{
    ui->setupUi(this);
    /* Plugin by default disabled, no injection, has QFrame, no injection process cmd */
    m_plugin = new Plugin(this, "NetProxy", this);

    m_proxySettings = new NetProxySettings(this);
    /* event to show the macro dialog */
    connect(ui->m_bt_settings, SIGNAL(clicked(bool)), m_proxySettings, SLOT(show()));
    /* send serial string */
    //connect(m_macroSettings, SIGNAL(sendCmd(QString)), this, SIGNAL(sendCmd(QString)));
    /* unload */
    connect(ui->m_bt_unload, SIGNAL(clicked(bool)), this, SLOT(removePlugin(bool)));
    /* handle LEDs */
    connect(m_proxySettings, SIGNAL(ledSetValue(NetProxySettings::en_led,bool)), this,
            SLOT(ledSetValue(NetProxySettings::en_led,bool)));
    /* data from netproxy -> plugin manager*/
    connect(m_proxySettings, SIGNAL(sendCmd(QString)), this, SIGNAL(sendCmd(QString)));
    /* data from plugin manager -> netproxy */
    connect(this, SIGNAL(proxyCmd(QByteArray)), m_proxySettings, SLOT(proxyCmd(QByteArray )));
    /* connect status labels */
    connect(m_proxySettings, SIGNAL(udpStatus(bool,QString)), this, SLOT(setUdpStatusText(bool,QString)));
    connect(m_proxySettings, SIGNAL(tcpStatus(bool,QString)), this, SLOT(setTcpStatusText(bool,QString)));
    ui->m_lbl_udp_status->setText("Not used");
    ui->m_lbl_tcp_status->setText("Not used");

    /* setup leds */
    m_leds = new NetActLed *[NetProxySettings::en_led::NUMBER_OF_LEDS] {
            NET_ACT_LED(NetProxySettings::en_led::LED_UDP_EN, ui->m_led_udp_enable),
            NET_ACT_LED(NetProxySettings::en_led::LED_UDP_TX, ui->m_led_udp_tx),
            NET_ACT_LED(NetProxySettings::en_led::LED_UDP_RX, ui->m_led_udp_rx),
            NET_ACT_LED(NetProxySettings::en_led::LED_TCP_EN, ui->m_led_tcp_enable),
            NET_ACT_LED(NetProxySettings::en_led::LED_TCP_TX, ui->m_led_tcp_tx),
            NET_ACT_LED(NetProxySettings::en_led::LED_TCP_RX, ui->m_led_tcp_rx),
    };

    for (size_t i=NetProxySettings::en_led::LED_UDP_EN; i<=NetProxySettings::en_led::LED_TCP_RX; i++) {
        if ((m_leds[i]->m_index != NetProxySettings::en_led::LED_UDP_EN) &&
               (m_leds[i]->m_index != NetProxySettings::en_led::LED_TCP_EN)) {
            m_leds[i]->m_tmr = new QTimer(m_leds[i]);
            connect(m_leds[i]->m_tmr, SIGNAL(timeout()), this, SLOT(tmrInterrupt()));
        }
    }

    connect(ui->m_cb_udp, &QCheckBox::clicked, this, [=](){
        QCheckBox * cb = qobject_cast<QCheckBox*>(sender());
        if (cb->isChecked()) {
            m_proxySettings->bindUdp();
        }
        else {
            m_proxySettings->unbindUdp();
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
const Plugin * NetProxyPlugin::plugin()
{
    return m_plugin;
}

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
    if ((led != NetProxySettings::en_led::LED_UDP_EN) &&
            (led != NetProxySettings::en_led::LED_TCP_EN)) {
        m_leds[led]->m_tmr->start(NetActLed::LED_INTERVAL_MS);
        TRACE << "[NetProxyPlugin::ledSetValue] timer started";
    }
}

void NetProxyPlugin::tmrInterrupt(void)
{
    QTimer * tmr = qobject_cast<QTimer*>(sender());
    NetActLed * led = qobject_cast<NetActLed*>(tmr->parent());

    led->m_icon->setPixmap(LED_GRAY);
}


void NetProxyPlugin::setUdpStatusText(bool state, QString text)
{
    if (state)
        ui->m_cb_udp->setCheckState(Qt::CheckState::Checked);
    else
        ui->m_cb_udp->setCheckState(Qt::CheckState::Unchecked);

    ui->m_lbl_udp_status->setText(text);
}

void NetProxyPlugin::setTcpStatusText(bool state, QString text)
{
    if (state)
        ui->m_cb_tcp->setCheckState(Qt::CheckState::Checked);
    else
        ui->m_cb_tcp->setCheckState(Qt::CheckState::Unchecked);

    ui->m_lbl_tcp_status->setText(text);
}

