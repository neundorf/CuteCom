#include "netproxysettings.h"
#include "ui_netproxysettings.h"
#include <QNetworkInterface>
#include <QMessageBox>
#include <QNetworkDatagram>
#include <QTextCodec>
#include <QPushButton>

#define TRACE if (!debug) {} else qDebug()
static bool debug = true;

NetProxySettings::NetProxySettings(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::NetProxySettings)
{
    ui->setupUi(this);

    ui->m_btn_udp->setText("Listen");
    ui->m_btn_tcp->setText("Connect");

    /* Set validators for the IP inputs */
    QString ipRange = "(?:[0-1]?[0-9]?[0-9]|2[0-4][0-9]|25[0-5])";
    // You may want to use QRegularExpression for new code with Qt 5 (not mandatory).
    QRegExp ipRegex ("^" + ipRange
                     + "\\." + ipRange
                     + "\\." + ipRange
                     + "\\." + ipRange + "$");
    QRegExpValidator *ipValidator = new QRegExpValidator(ipRegex, this);
    ui->m_le_udp_remote_host->setValidator(ipValidator);
    ui->m_le_tcp_remote_host->setValidator(ipValidator);


    /* Initialize UDP socket */
    m_udp = new QUdpSocket(this);
    connect(ui->m_btn_udp, &QPushButton::clicked, this, [=](){
        TRACE << "UDP state: " << m_udp->state();
        if (m_udp->state() != QAbstractSocket::BoundState) {
           bindUdp();
        }
        else {
           unbindUdp();
        }
    });
    getLocalIp();

    connect(ui->m_bt_udp_help, &QPushButton::clicked, this, &NetProxySettings::helpMsgUdp);
    connect(ui->m_bt_tcp_help, &QPushButton::clicked, this, &NetProxySettings::helpMsgTcp);
}

NetProxySettings::~NetProxySettings()
{
    delete ui;
}

void NetProxySettings::bindUdp()
{
    QHostAddress r_addr(ui->m_le_udp_remote_host->text());
    QHostAddress l_addr(ui->m_cb_udp_local_ip->currentText());

    /* Do some checks */
    if (QAbstractSocket::IPv4Protocol != r_addr.protocol()) {
        QMessageBox::warning(this, "Error", "Invalid remote IP address: " + r_addr.toString());
        return;
    }
    if (QAbstractSocket::IPv4Protocol != l_addr.protocol()) {
        QMessageBox::warning(this, "Error", "Invalid local IP address: " + l_addr.toString());
        return;
    }

    int l_port = ui->m_sb_udp_port_local->text().toInt();
    if (!l_port || l_port > 65535) {
        QMessageBox::warning(this, "Error", "Invalid local port range!");
        return;
    }
    int r_port = ui->m_sb_udp_port_remote->text().toInt();
    if (!r_port || r_port > 65535) {
        QMessageBox::warning(this, "Error", "Invalid remote port range!");
        return;
    }
    if (l_port == r_port) {
        QMessageBox::warning(this, "Error", "Remote and local port must be different, otherwise it will create a loop!");
        return;
    }

    if (m_udp->bind(l_addr, l_port)) {
        connect(m_udp, SIGNAL(readyRead()), this, SLOT(recvUDP()));
        connect(m_udp, SIGNAL(error(QAbstractSocket::SocketError)), this, SLOT(errorUdp(QAbstractSocket::SocketError)));
        ui->m_btn_udp->setText("Close");
        /* store udp details */
        m_udp_remote_addr = r_addr;
        m_udp_remote_port = r_port;
        emit ledSetValue(en_led::LED_UDP_EN,true);
        QString status(l_addr.toString() + ":" + QString::number(l_port));
        emit udpStatus(true, status);
        TRACE << "[NetProxySettings] UDP bind " << status;
    }
    else {
        QMessageBox::warning(this, "Error", "Could not bind UDP socket!");
        m_udp_remote_addr.clear();
    }
}

void NetProxySettings::unbindUdp()
{
    m_udp->close();
    m_udp_remote_addr.clear();
    m_udp_remote_port = 0;
    ui->m_btn_udp->setText("Listen");
    emit ledSetValue(en_led::LED_UDP_EN, false);
    emit udpStatus(false, QString("Not used"));
    TRACE << "[NetProxySettings] UDP unbind";
}

void NetProxySettings::errorUdp(QAbstractSocket::SocketError err)
{
    unbindUdp();
    if (err) {
        QMessageBox::warning(this, "Error", "UDP socket error: " + m_udp->errorString());
    }
}

void NetProxySettings::recvUDP()
{
    while (m_udp->hasPendingDatagrams()) {
        QNetworkDatagram datagram = m_udp->receiveDatagram();
        QString str_data = QString::fromUtf8(datagram.data());
        emit sendCmd(str_data);
        TRACE << "[NetProxySettings::recvUDP] udp from host: " << str_data;
        emit ledSetValue(en_led::LED_UDP_RX, true);
    }
}

void NetProxySettings::proxyCmd(QByteArray cmd)
{
    if (m_udp->state() == QAbstractSocket::BoundState) {
        m_udp->writeDatagram(cmd, m_udp_remote_addr, m_udp_remote_port);
        emit ledSetValue(en_led::LED_UDP_TX, true);
    }
    TRACE << "[NetProxySettings::proxyCmd]: " << QString::fromUtf8(cmd.data());
}


void NetProxySettings::getLocalIp()
{
    QList<QHostAddress> list = QNetworkInterface::allAddresses();

    for(int i=0; i<list.count(); i++)
    {
        if (list[i].isLoopback())
            continue;
        if (list[i].protocol() == QAbstractSocket::IPv4Protocol ) {
            TRACE << "[NetProxySettings] found interface: " << list[i].toString();
            ui->m_cb_udp_local_ip->addItem(list[i].toString());
        }
    }
}



void NetProxySettings::helpMsgUdp(void)
{
    QString help_str = "This plugin provides a UDP socket that can be\n"
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
                       "an infinite tx/rx loop\n";

    QMessageBox::information(this, "How to use UDP forwarding", help_str);
}


void NetProxySettings::helpMsgTcp(void)
{
    QString help_str = "This plugin provides a TCP socket that can be\n"
                       "used to forward the incoming serial data to a\n"
                       "remote socket or the opposite. Therefore, the TCP\n"
                       "communication is bi-directional.\n\n"
                       "The TCP socket will listen to the local port\n"
                       "and forward the traffic of this port to the serial\n"
                       "port.\n\n"
                       "Also the TCP socket will forward all the incoming\n"
                       "data from the serial device to the remote TCP port\n"
                       "and IP which is set in the settings.\n\n";

    QMessageBox::information(this, "How to use TCP forwarding", help_str);
}
