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

#ifndef SETTINGS_H
#define SETTINGS_H

#include <QObject>
#include <QtSerialPort>

class Settings : public QObject
{
    Q_OBJECT

public:
    enum Options {
        Device,
        BaudRate,
        DataBits,
        StopBits,
        Parity,
        FlowControl,
        OpenMode,
        ShowCtrlCharacters,
        ShowTimestamp,
        CommandHistory,
        WindowGeometry,
        WindowState,
        LogFileLocation,
        LineTermination,
        CharacterDelay,
        SendStartDir,
        ProtocolOption,
        CurrentSession
    };

    struct Session {
        QString device;
        quint32 baudRate;
        QSerialPort::DataBits dataBits;
        QSerialPort::Parity parity;
        QSerialPort::StopBits stopBits;
        QSerialPort::FlowControl flowControl;
        QIODevice::OpenModeFlag openMode;
        bool showCtrlCharacters;
        bool showTimestamp;
        QStringList command_history;
    };

    enum LineTerminator { LF = 0, CR, CRLF, NONE, HEX };
    Q_ENUMS(LineTerminator)

    enum Protocol { PLAIN, SCRIPT, XMODEM, YMODEM, ZMODEM, ONEKXMODEM, PROTOCOL_MAX };
    Q_ENUMS(Protocol)

    explicit Settings(QObject *parent = 0);
    void readSettings(const QString &session);

    const Settings::Session getCurrentSession();
    QString getCurrentSessionName() const { return m_current_session; }
    void settingChanged(Settings::Options option, QVariant setting);

    QByteArray getWindowGeometry() const;

    QByteArray getWindowState() const;

    QString getLogFileLocation() const;

    Settings::LineTerminator getLineTerminator() const;

    quint8 getCharacterDelay() const { return m_character_delay; }

    Settings::Protocol getProtocol() const { return m_protocol; }

    QString getSendStartDir() const { return m_sendingStartDir; }

    QList<QString> getSessionNames() const;

    void removeSession(const QString &session);

    void cloneSession(const QString &source, const QString &destination);

    void renameSession(const QString &source, const QString &destination);

signals:
    void sessionChanged(const Settings::Session &);

private:
    void readSessionSettings(QSettings &settings);
    void saveGenericSettings();
    void saveSessionSettings();
    bool readUIntSetting(QSettings &settings, QString const &name, quint32 *i);

    QByteArray m_windowGeometry;
    QByteArray m_windowState;
    QString m_logFileLocation;
    /**
     * The location QFileDialog displayed for choosing a file
     * to send starts off
     * @brief m_sendingStartDir
     */
    QString m_sendingStartDir;
    Settings::LineTerminator m_lineterm;

    /**
     * This holds the last protocol selected for
     * sending a file across the device
     * @brief m_protocol
     */
    Settings::Protocol m_protocol;

    /**
     * Delay between each character sent
     * @brief m_character_delay;
     */
    quint8 m_character_delay;

    QHash<QString, Session> m_sessions;
    QString m_current_session;
    static const QString DEFAULT_SESSION_NAME;
};

Q_DECLARE_METATYPE(Settings::Session)
Q_DECLARE_METATYPE(Settings::LineTerminator)
Q_DECLARE_METATYPE(Settings::Protocol)

#endif // SETTINGS_H
