#ifndef COUNTERPLUGIN_H
#define COUNTERPLUGIN_H

#include "plugin.h"
#include "settings.h"
#include <QDebug>
#include <QFrame>

namespace Ui
{
class CounterPlugin;
}

class CounterPlugin : public QFrame
{
    Q_OBJECT

public:
    explicit CounterPlugin(QFrame *parent, Settings *settings);
    ~CounterPlugin();
    const Plugin *plugin();
    //    int processCmd(const QString *text, QString *new_text);

signals:
    void sendCmd(QByteArray);
    void unload(Plugin *);

public slots:
    void txBytes(int);
    void rxBytes(QByteArray);
    void removePlugin(bool);
    void helpMsg(void);

private:
    Ui::CounterPlugin *ui;
    Plugin *m_plugin;
    Settings *m_settings;
};

#endif // COUNTERPLUGIN_H
