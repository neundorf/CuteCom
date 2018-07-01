#ifndef NETPROXYSETTINGS_H
#define NETPROXYSETTINGS_H

#include <QDialog>
#include <QUdpSocket>

namespace Ui {
class NetProxySettings;
}

class NetProxySettings : public QDialog
{
    Q_OBJECT

public:
    enum en_led {
        LED_UDP_EN,
        LED_UDP_TX,
        LED_UDP_RX,
        LED_TCP_EN,
        LED_TCP_TX,
        LED_TCP_RX,
        NUMBER_OF_LEDS
    };
    explicit NetProxySettings(QWidget *parent = 0);
    ~NetProxySettings();

signals:
    void sendCmd(QString);
    void ledSetValue(NetProxySettings::en_led, bool);
    void udpStatus(bool, QString);
    void tcpStatus(bool, QString);

public slots:
    void proxyCmd(QByteArray); /* data netproxy plugin -> UDP */
    void bindUdp();
    void unbindUdp();
    void errorUdp(QAbstractSocket::SocketError);
    void recvUDP();

private:
    Ui::NetProxySettings *ui;
    QUdpSocket * m_udp;
    QHostAddress m_udp_remote_addr;
    qint16 m_udp_remote_port;
    void helpMsgUdp(void);
    void helpMsgTcp(void);

    void getLocalIp();
};

#endif // NETPROXYSETTINGS_H
