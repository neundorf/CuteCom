#ifndef NETPROXYPLUGIN_H
#define NETPROXYPLUGIN_H

#include <QDebug>
#include <QFrame>
#include <QLabel>
#include "settings.h"
#include "plugin.h"
#include "netproxysettings.h"

namespace Ui {
class NetProxyPlugin;
}

class NetActLed : public QObject
{
    Q_OBJECT
public:
    enum {LED_INTERVAL_MS = 250};
    NetActLed(NetProxySettings::en_led led_index, QLabel * led_icon, QTimer * tmr = 0) :
        m_index(led_index),
        m_icon(led_icon),
        m_tmr(tmr) {}
    NetProxySettings::en_led m_index;
    QLabel * m_icon;
    QTimer * m_tmr;
};

class NetProxyPlugin : public QFrame
{
    Q_OBJECT

public:
    explicit NetProxyPlugin(QFrame *parent = 0, Settings * settings = 0);
    virtual ~NetProxyPlugin();
    const Plugin * plugin();
    int processCmd(const QString * text);

signals:
    void sendCmd(QString);  /* netproxy -> plugin manager */
    void proxyCmd(QByteArray); /* plugin manager -> netproxy */
    void unload(Plugin*);

private slots:
    void removePlugin(bool);
    void ledSetValue(NetProxySettings::en_led, bool);
    void tmrInterrupt(void);
    void setUdpStatusText(bool, QString);
    void setTcpStatusText(bool, QString);

private:
    Settings * m_settings;
    Ui::NetProxyPlugin *ui;
    Plugin *m_plugin;
    NetProxySettings * m_proxySettings;
    NetActLed ** m_leds;
};

#endif // NETPROXYPLUGIN_H
