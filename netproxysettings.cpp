#include "netproxysettings.h"
#include "ui_netproxysettings.h"
#include <QCloseEvent>
#include <QMessageBox>
#include <QNetworkDatagram>
#include <QNetworkInterface>
#include <QPushButton>
#include <QTextCodec>

#define TRACE                                                                                                          \
    if (!debug) {                                                                                                      \
    } else                                                                                                             \
        qDebug()
static bool debug = true;

NetProxySettings::NetProxySettings(Settings *settings, QWidget *parent)
    : QDialog(parent)
    , m_settings(settings)
    , ui(new Ui::NetProxySettings)
{
    ui->setupUi(this);

    ui->m_btn_udp->setText("Listen");
    ui->m_btn_tcp->setText("Listen");

    /* Set validators for the IP inputs */
    QString ipRange = "(?:[0-1]?[0-9]?[0-9]|2[0-4][0-9]|25[0-5])";
    // You may want to use QRegularExpression for new code with Qt 5 (not mandatory).
    QRegExp ipRegex("^" + ipRange + "\\." + ipRange + "\\." + ipRange + "\\." + ipRange + "$");
    QRegExpValidator *ipValidator = new QRegExpValidator(ipRegex, this);
    ui->m_le_udp_remote_host->setValidator(ipValidator);

    getLocalIp();

    /* Initialize UDP socket */
    m_udp = new QUdpSocket(this);
    connect(ui->m_btn_udp, &QPushButton::clicked, this, [=]() {
        TRACE << "UDP state: " << m_udp->state();
        if (m_udp->state() != QAbstractSocket::BoundState) {
            bindUdp();
        } else {
            unbindUdp();
        }
    });
    connect(ui->m_bt_udp_help, &QPushButton::clicked, this, &NetProxySettings::helpMsgUdp);
    connect(ui->m_bt_tcp_help, &QPushButton::clicked, this, &NetProxySettings::helpMsgTcp);
    connect(m_udp, SIGNAL(error(QAbstractSocket::SocketError)), this, SLOT(errorSocket(QAbstractSocket::SocketError)));

    /* Initialise TCP socket */
    m_tcp = new QTcpServer(this);
    connect(ui->m_btn_tcp, &QPushButton::clicked, this, [=]() {
        TRACE << "TCP state: " << m_udp->state();
        if (m_tcp->isListening()) {
            stopTcpServer();
        } else {
            startTcpServer();
        }
    });
    connect(m_tcp, SIGNAL(newConnection()), this, SLOT(addTcpClient()));
    connect(m_tcp, SIGNAL(acceptError(QAbstractSocket::SocketError)), this,
            SLOT(errorSocket(QAbstractSocket::SocketError)));

    /* update controls with the saved settings */
    ui->m_sb_udp_port_local->setValue(m_settings->getCurrentSession().udpLocalPort);
    ui->m_le_udp_remote_host->setText(m_settings->getCurrentSession().udpRemoteHost);
    ui->m_sb_udp_port_remote->setValue(m_settings->getCurrentSession().udpRemotePort);
    ui->m_sb_tcp_port_local->setValue(m_settings->getCurrentSession().tcpLocalPort);
    connect(this, SIGNAL(rejected()), this, SLOT(formClose()));
}

NetProxySettings::~NetProxySettings() { delete ui; }

/**
 * @brief When dialog closes then save settings
 */
void NetProxySettings::formClose()
{
    /* update the settings with the current values */
    m_settings->settingChanged(Settings::UdpLocalPort, ui->m_sb_udp_port_local->value());
    m_settings->settingChanged(Settings::UdpRemoteHost, ui->m_le_udp_remote_host->text());
    m_settings->settingChanged(Settings::UdpRemotePort, ui->m_sb_udp_port_remote->value());
    m_settings->settingChanged(Settings::TcpLocalPort, ui->m_sb_tcp_port_local->value());
    TRACE << "[NetProxySettings::formClose]";
}

/**
 * @brief Check for valid IPv4 address
 * @param addr The address to check
 * @return true if IPv4 else false
 */
bool NetProxySettings::CheckIpAddress(QHostAddress *addr)
{
    if (QAbstractSocket::IPv4Protocol != addr->protocol()) {
        QMessageBox::critical(this, tr("Error"), tr("Invalid remote IP address: %1.").arg(addr->toString()));
        return false;
    }
    return true;
}

/**
 * @brief Check for valid TCP/UDP port
 * @param port The port number. All port numbers are allowed
 * @return true if valid port, else false
 */
bool NetProxySettings::CheckPort(quint16 port)
{
    if (!port) {
        QMessageBox::warning(this, tr("Error"), tr("Invalid remote port range!"));
        return false;
    }
    return true;
}

/**
 * @brief UDP socket errors
 * @param err The error that occured
 */
void NetProxySettings::errorSocket(QAbstractSocket::SocketError err)
{
    unbindUdp();
    if (err) {
        QMessageBox::critical(this, tr("Error"), tr("UDP socket error: %1.").arg(m_udp->errorString()));
    }
}

/**
 * @brief Bind UDP address and port and notify the interface
 *  elements.
 */
void NetProxySettings::bindUdp()
{
    QHostAddress r_addr(ui->m_le_udp_remote_host->text());
    QHostAddress l_addr(ui->m_cb_udp_local_ip->currentText());

    /* Do some checks */
    if (!CheckIpAddress(&r_addr) || !CheckIpAddress(&l_addr))
        return;

    int l_port = ui->m_sb_udp_port_local->text().toInt();
    int r_port = ui->m_sb_udp_port_remote->text().toInt();
    if (!CheckPort(l_port) || !CheckPort(r_port))
        return;

    if (l_port == r_port) {
        QMessageBox::warning(this, tr("Error"),
                             tr("Remote and local port must be different, otherwise it will create a loop!"));
        return;
    }

    if (m_udp->bind(l_addr, l_port)) {
        connect(m_udp, SIGNAL(readyRead()), this, SLOT(recvUDP()));
        connect(m_udp, SIGNAL(error(QAbstractSocket::SocketError)), this, SLOT(errorUdp(QAbstractSocket::SocketError)));
        ui->m_btn_udp->setText("Close");
        /* store udp details */
        m_udp_remote_addr = r_addr;
        m_udp_remote_port = r_port;
        emit ledSetValue(en_led::LED_UDP_EN, true);
        QString status(QString("%1 : %2").arg(l_addr.toString()).arg(QString::number(l_port)));
        emit udpStatus(true, status);
        TRACE << "[NetProxySettings] UDP bind " << status;
    } else {
        QMessageBox::critical(this, tr("Error"), tr("Could not bind UDP socket!"));
        m_udp_remote_addr.clear();
    }
}

/**
 * @brief In case of error or close request, then close the
 *  socket and notify the user interface elements
 */
void NetProxySettings::unbindUdp()
{
    m_udp->close();
    m_udp_remote_addr.clear();
    m_udp_remote_port = 0;
    ui->m_btn_udp->setText(tr("Listen"));
    emit ledSetValue(en_led::LED_UDP_EN, false);
    emit udpStatus(false, QString(tr("Not used")));
    TRACE << "[NetProxySettings] UDP unbind";
}

/**
 * @brief [SIGNAL] This receives the data from UDP and then
 *  emits the received data.
 */
void NetProxySettings::recvUDP()
{
    while (m_udp->hasPendingDatagrams()) {
        emit sendCmd(m_udp->receiveDatagram().data());
        emit ledSetValue(en_led::LED_UDP_RX, true);
    }
}

/**
 * @brief Start the local TCP server
 */
void NetProxySettings::startTcpServer()
{
    int l_port = ui->m_sb_udp_port_local->text().toInt();
    if (!CheckPort(l_port))
        return;
    if (!m_tcp->listen(QHostAddress::Any, l_port)) {
        QMessageBox::critical(this, tr("Error"), tr("Could not start TCP server.\n%1.").arg(m_tcp->errorString()));
        return;
    }
    ui->m_btn_tcp->setText(tr("Close"));
    emit ledSetValue(en_led::LED_TCP_EN, true);
    QString status(tr("Listening on: %1").arg(QString::number(l_port)));
    emit tcpStatus(true, status);
}

/**
 * @brief Stop local TCP server
 */
void NetProxySettings::stopTcpServer()
{
    m_tcp->close();
    ui->m_btn_tcp->setText(tr("Listen"));
    emit ledSetValue(en_led::LED_TCP_EN, false);
    emit tcpStatus(false, QString(tr("Not used")));
}

/**
 * @brief On every new TCP connection add the client to the list
 */
void NetProxySettings::addTcpClient()
{
    QTcpSocket *client = m_tcp->nextPendingConnection();
    connect(client, SIGNAL(disconnected()), this, SLOT(removeTcpClient()));
    connect(client, SIGNAL(readyRead()), this, SLOT(recvTCP()));
    /* add client to the list */
    m_tcpClients.append(client);
    TRACE << "Connected: " << client->localAddress();
}

/**
 * @brief Receive data from TCP clients and forward the data to the serial
 */
void NetProxySettings::recvTCP()
{
    QTcpSocket *client = qobject_cast<QTcpSocket *>(sender());
    while (client->bytesAvailable()) {
        QByteArray recv_data = client->readAll();
        emit sendCmd(recv_data);
        emit ledSetValue(en_led::LED_TCP_RX, true);
        TRACE << "TCP in: " << recv_data;
    }
}

/**
 * @brief Remove clients from the server's list
 */
void NetProxySettings::removeTcpClient()
{
    QTcpSocket *client = qobject_cast<QTcpSocket *>(sender());
    client->disconnectFromHost();
    m_tcpClients.removeOne(client);
    TRACE << "Disconnected: " << client->localAddress();
}

/**
 * @brief Receive data from the plugin and send them to the
 *  UDP socket.
 * @param cmd The data bytes
 */
void NetProxySettings::proxyCmd(QByteArray cmd)
{
    if (m_udp->state() == QAbstractSocket::BoundState) {
        m_udp->writeDatagram(cmd, m_udp_remote_addr, m_udp_remote_port);
        emit ledSetValue(en_led::LED_UDP_TX, true);
    }
    if (m_tcp->isListening()) {
        /* send the data to all clients */
        QTcpSocket *client;
        foreach (client, m_tcpClients) {
            client->write(cmd);
            emit ledSetValue(en_led::LED_TCP_TX, true);
        }
    }
    TRACE << "[NetProxySettings::proxyCmd]: " << QString::fromUtf8(cmd.data());
}

/**
 * @brief Retrieve the IP addresses of all the local interfaces
 */
void NetProxySettings::getLocalIp()
{
    QList<QHostAddress> list = QNetworkInterface::allAddresses();

    for (int i = 0; i < list.count(); i++) {
        if (list[i].isLoopback())
            continue;
        if (list[i].protocol() == QAbstractSocket::IPv4Protocol) {
            TRACE << "[NetProxySettings] found interface: " << list[i].toString();
            ui->m_cb_udp_local_ip->addItem(list[i].toString());
        }
    }
}

/**
 * @brief Help message for UDP settings
 */
void NetProxySettings::helpMsgUdp(void)
{
    QString help_str = tr("This plugin provides a UDP socket that can be\n"
                          "used to forward the incoming serial data to the\n"
                          "bind socket or the opposite. Therefore, the UDP\n"
                          "communication is bi-directional.\n\n"
                          "The UDP socket will bind the local listen port\n"
                          "and forward the traffic of this port to the serial\n"
                          "port.\n\n"
                          "Also the UDP socket will forward all the incoming\n"
                          "data from the serial device to the remote UDP port\n"
                          "and IP which is set in the settings.\n\n"
                          "For obvious reasons, using the same UDP port for\n"
                          "listen and send is not allowed as it may lead to\n"
                          "an infinite tx/rx loop\n");

    QMessageBox::information(this, tr("How to use UDP forwarding"), help_str);
}

/**
 * @brief Help message for TCP settings
 */
void NetProxySettings::helpMsgTcp(void)
{
    QString help_str = tr("This plugin provides a TCP server that can be\n"
                          "used to forward the incoming serial data to and from\n"
                          "connected clients.\n\n"
                          "Select the local port and run the server. Then use\n"
                          "any TCP client (e.g. telnet) to connect to the server\n"
                          "and receive or send data in the serial port.\n");

    QMessageBox::information(this, tr("How to use TCP forwarding"), help_str);
}
