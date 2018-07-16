#ifndef NETPROXYSETTINGS_H
#define NETPROXYSETTINGS_H

#include "settings.h"
#include <QDialog>
#include <QTcpServer>
#include <QTcpSocket>
#include <QUdpSocket>

namespace Ui
{
class NetProxySettings;
}

class NetProxySettings : public QDialog
{
    Q_OBJECT

public:
    enum en_led { LED_UDP_EN, LED_UDP_TX, LED_UDP_RX, LED_TCP_EN, LED_TCP_TX, LED_TCP_RX, NUMBER_OF_LEDS };
    explicit NetProxySettings(Settings *settings, QWidget *parent = 0);
    ~NetProxySettings();

signals:
    void sendCmd(QByteArray);
    void ledSetValue(NetProxySettings::en_led, bool);
    void udpStatus(bool, QString);
    void tcpStatus(bool, QString);

public slots:
    void proxyCmd(QByteArray); /* data netproxy plugin -> UDP */
    void bindUdp();
    void unbindUdp();
    void startTcpServer();
    void stopTcpServer();

private slots:
    void formClose();
    void recvUDP();
    void addTcpClient();
    void removeTcpClient();
    void recvTCP();
    void errorUdpSocket(QAbstractSocket::SocketError);
    void errorTcpSocket(QAbstractSocket::SocketError);

private:
    Settings *m_settings;
    Ui::NetProxySettings *ui;
    QUdpSocket *m_udp;
    QHostAddress m_udp_remote_addr;
    qint16 m_udp_remote_port;
    QTcpServer *m_tcp;
    QList<QTcpSocket *> m_tcpClients;

    void helpMsgUdp(void);
    void helpMsgTcp(void);
    void getLocalIp();
    bool CheckIpAddress(QHostAddress *addr);
    bool CheckPort(quint16 port);
};

#endif // NETPROXYSETTINGS_H
