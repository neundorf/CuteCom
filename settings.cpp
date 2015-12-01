/*
 * Copyright (c) 2015 Meinhard Ritscher <cyc1ingsir@gmail.com>
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
#include "settings.h"

#include <QSettings>
#include <QHash>
#include <QVariant>
#include <QSerialPort>

#include <qdebug.h>

const QString Settings::DEFAULT_SESSION_NAME = QStringLiteral( "Default");

Settings::Settings(QObject* parent) : QObject(parent) {}

void Settings::settingChanged(Settings::Options option, QVariant setting)
{
    bool sessionSettings = true;
    Session session;
    if (m_sessions.contains(m_current_session))
            session = m_sessions.value(m_current_session);

    switch (option) {
    case BaudRate:
        session.baudRate = setting.toInt();
        break;
    case StopBits:
        session.stopBits = static_cast<QSerialPort::StopBits>(setting.toInt());
        break;
    case DataBits:
        session.dataBits = static_cast<QSerialPort::DataBits>(setting.toInt());
        break;
    case Parity:
        session.parity = static_cast<QSerialPort::Parity>(setting.toInt());
        break;
    case FlowControl:
        session.flowControl = static_cast<QSerialPort::FlowControl>(setting.toInt());
        break;
    case OpenMode:
        session.openMode = static_cast<QIODevice::OpenModeFlag>(setting.toInt());
        break;
    case Device:
        session.device = setting.toString();
        break;
    case ShowCtrlCharacters:
        session.showCtrlCharacters = setting.toBool();
        break;
    case ShowTimestamp:
        session.showTimestamp = setting.toBool();
        break;
    case CommandHistory:
        session.command_history = setting.toStringList();
        break;
    case WindowGeometry:
        m_windowGeometry = setting.toRect();
        sessionSettings = false;
        break;
    case LogFileLocation:
        m_logFileLocation = setting.toString();
        sessionSettings = false;
        break;
    case LineTermination:
        m_lineterm = setting.value<LineTerminator>();
        sessionSettings = false;
        break;
    case CharacterDelay:
        m_character_delay = setting.toUInt();
        sessionSettings = false;
        break;
    case ProtocolOption:
        m_protocol = setting.value<Protocol>();
        sessionSettings = false;
        break;
    case SendStartDir:
        m_sendingStartDir = setting.toString();
        sessionSettings = false;
        break;
    case CurrentSession:
        m_current_session = setting.toString();
        emit sessionChanged(getCurrentSession());
        sessionSettings = false;
    default:
        break;
    }
    m_sessions.insert(m_current_session, session);
    if(sessionSettings) {
        saveSessionSettings();
        emit sessionChanged(getCurrentSession());
    } else {
        saveGenericSettings();
    }
}

/**
 * @brief Settings::readSessionSettings
 * Reads the settings of all stored sessions.
 * These settings contain settings relevant to connect to a specific device
 * especially the baud rate but also the command history which may be device
 * specific as well.
 * If the value for a setting can't be successfully been read, a default value
 * will be used.
 * If that value isn't beeing changed by the user, the setting will not be
 * rewritten to the config file. But this doesn't effect the user since the
 * default values for this setting seem to be suit him/her.
 *
 * @param settings a reference to the QSetting instance of this process
 */
void Settings::readSessionSettings(QSettings &settings)
{
    int size = settings.beginReadArray("sessions");

    if( size < 1) {
        qDebug() << "no session information found in conf file";
        return;
    }
    m_sessions.clear();
    quint32 value;

    for( int i = 0; i < size; i++) {
        settings.setArrayIndex(i);
        Session session;
        QString name = settings.value("name").toString();
        if(name.isEmpty())
            continue;

        /*
         * the static casts are prone to errors since a valid int in the config
         * file might not be able to be mapped to a QSerialPort enum
         * e.g. flowControl=35600 has no equivalent
         * the static cast can't throw an error but this error will be
         * handled the time this value is appied to the combo boxes within
         * the control panel. It's a workaround though.
         */
        session.baudRate = settings.value("BaudRate",115200).toInt();
        session.dataBits = (readUIntSetting(settings, QStringLiteral("DataBits"), &value))?
                    static_cast<QSerialPort::DataBits>(value) : QSerialPort::Data8;
        session.parity = (readUIntSetting(settings, QStringLiteral("Parity"), &value))?
                    static_cast<QSerialPort::Parity>(value) : QSerialPort::NoParity;
        session.stopBits = (readUIntSetting(settings, QStringLiteral("StopBits"), &value))?
                    static_cast<QSerialPort::StopBits>(value) : QSerialPort::OneStop;
        session.flowControl = (readUIntSetting(settings, QStringLiteral("FlowControl"), &value))?
                    static_cast<QSerialPort::FlowControl>(value) : QSerialPort::NoFlowControl;
        session.openMode = (readUIntSetting(settings, QStringLiteral("OpenMode"), &value))?
                    static_cast<QIODevice::OpenModeFlag>(value) : QIODevice::ReadWrite;
        // the recovering default for the device its Linux specific but I can live with that
        session.device = settings.value("Device", QStringLiteral("/dev/ttyUSB0")).toString();
        session.showCtrlCharacters = settings.value("showCtrlCharacters", false).toBool();
        session.showTimestamp = settings.value("showTimestamp", false).toBool();
        session.command_history = settings.value("History").toStringList();

        m_sessions.insert(name, session);
    }
    settings.endArray();

}

bool Settings::readUIntSetting(QSettings &settings, QString const& name, quint32 *i )
{
    bool ok = false;
    int r = settings.value(name, -1).toInt(&ok);
    if( r < 0 || !ok) {
        *i = 0;
        return false;
    } else {
        *i = r;
    }
    return true;
}

/**
 * @brief Settings::readSettings
 * @param session A specific session to be used as the current session.
 */
void Settings::readSettings(const QString &session)
{
    QSettings settings(this->parent());
    settings.beginGroup("CuteCom");
    QString stored_session = settings.value("session",DEFAULT_SESSION_NAME).toString();
    if(session.isEmpty()){
        m_current_session = stored_session;
    } else {
            m_current_session = session;
            if(session != stored_session)
                settings.setValue("session", session);
    }
    qDebug() << "setting current session to: " << m_current_session;

    m_windowGeometry = settings.value("WindowGeometry", QRect(0,0,0,0)).toRect();
    m_logFileLocation = settings.value("LogFileLocation").toString();
    if (m_logFileLocation.isEmpty()) {
        m_logFileLocation = QDir::homePath() + QDir::separator() + QStringLiteral("cutecom.log");
    }

    m_lineterm = static_cast<Settings::LineTerminator>(settings.value("LineTerminator", QVariant::fromValue(Settings::LF)).toUInt());

    m_protocol = static_cast<Settings::Protocol>(settings.value("Protocol", QVariant::fromValue(Settings::PLAIN)).toUInt());

    m_sendingStartDir = settings.value("SendingStartDir",QDir::homePath()).toString();

    m_character_delay = settings.value("CharacterDelay", 0).toUInt();

    settings.endGroup();
    readSessionSettings(settings);
}

const Settings::Session Settings::getCurrentSession()
{
    if (!m_sessions.contains(m_current_session)) {
        // Since this is probably the first time CuteCom is
        // started (with this session specified as parameter)
        // we set sensible default values for at least
        // the connection parameter

        Settings::Session session;
        session.baudRate = 115200;
        session.dataBits = QSerialPort::Data8;
        session.parity = QSerialPort::NoParity;
        session.stopBits = QSerialPort::OneStop;
        session.openMode = QIODevice::ReadWrite;
        session.flowControl = QSerialPort::NoFlowControl;
        m_sessions.insert(m_current_session, session);
    }

    return m_sessions.value(m_current_session);
}

void Settings::saveGenericSettings()
{
    QSettings settings(this->parent());
    settings.beginGroup("CuteCom");
    // store generic fluff
    settings.setValue("WindowGeometry", m_windowGeometry);
    settings.setValue("LogFileLocation", m_logFileLocation);

    // save session releated settings
    if(!m_current_session.isEmpty())
        settings.setValue("session", m_current_session );
    else
        settings.setValue("session", DEFAULT_SESSION_NAME);

    settings.setValue("LineTerminator", m_lineterm);

    settings.setValue("CharacterDelay", m_character_delay);

    settings.setValue("Protocol", m_protocol);

    settings.setValue("SendingStartDir", m_sendingStartDir);

    settings.endGroup();

}

void Settings::saveSessionSettings()
{
    QSettings settings(this->parent());

    if(m_sessions.size()> 0) {
        settings.beginWriteArray("sessions");
        int index = 0;
        QHashIterator<QString, Session> iter(m_sessions);
        while (iter.hasNext()) {
            iter.next();
            settings.setArrayIndex(index++);
            settings.setValue("name", iter.key());
            Session session = iter.value();
            settings.setValue("BaudRate", session.baudRate);
            settings.setValue("DataBits", session.dataBits);
            settings.setValue("Parity", session.parity);
            settings.setValue("StopBits", session.stopBits);
            settings.setValue("FlowControl", session.flowControl);
            settings.setValue("OpenMode", session.openMode);
            settings.setValue("Device", session.device);
            settings.setValue("showCtrlCharacters", session.showCtrlCharacters);
            settings.setValue("showTimestamp", session.showTimestamp);
            settings.setValue("History", session.command_history);
        }
        settings.endArray();
    }

}

void Settings::removeSession(const QString &session)
{
    m_sessions.remove(session);
    QSettings settings;
    settings.beginGroup("sessions");
    settings.remove("");
    settings.endGroup();
    saveSessionSettings();
    m_current_session = QStringLiteral("Default");
    saveGenericSettings();
    emit this->sessionChanged(getCurrentSession());
}

void Settings::cloneSession(const QString &source, const QString &destination)
{

}

void Settings::renameSession(const QString &source, const QString &destination)
{

}

QString Settings::getLogFileLocation() const
{
    return m_logFileLocation;
}

Settings::LineTerminator Settings::getLineTerminator() const
{
    return m_lineterm;
}

QList<QString> Settings::getSessionNames() const
{
    QList<QString> sessions =  m_sessions.keys();
    if(sessions.isEmpty())
        sessions.append(QStringLiteral("Default"));
    return sessions;
}

QRect Settings::getWindowGeometry() const
{
    return m_windowGeometry;
}
