/*
 * Copyright (C) 2004-2009 Alexander Neundorf <neundorf@kde.org> (code used from original CuteCom)
 * Copyright (C) 2013 Preet Desai <preet.desai@gmail.com> (code to send files proted to Qt5 for the original CuteCom
 *                                                          from https://github.com/preet/cutecom-qt5)
 * Copyright (c) 2015 Meinhard Ritscher <cyc1ingsir@gmail.com>
 * Copyright (c) 2015 Antoine Calando <acalando@free.fr> (improvements added to original CuteCom)
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

#include "mainwindow.h"
#include "version.h"
#include "settings.h"
#include "qdebug.h"

#include <QTime>
#include <QTimer>
#include <QThread>
#include <QFileDialog>
#include <QResizeEvent>
#include <QMessageBox>
#include <QSpinBox>
#include <QScrollBar>
#include <QProgressDialog>
#include <QtSerialPort/QSerialPort>
#include <QtSerialPort/QSerialPortInfo>

void millisleep(unsigned long ms)
{
   if (ms>0)
   {
       QThread::msleep(ms);
   }
}

MainWindow::MainWindow(QWidget *parent, const QString &session)
   :QMainWindow(parent)
   ,m_progress(0)
   ,m_sz(0)
   ,m_previousChar('\0')
   ,m_keyRepeatTimer(this)
   ,m_keyCode(0)
   ,m_cmdBufIndex(0)
{
    QCoreApplication::setOrganizationName(QStringLiteral("CuteCom"));
    // setting it to CuteCom5 will prevent the original CuteCom's settings file
    // to be overwritten
    QCoreApplication::setApplicationName(QStringLiteral("CuteCom5"));
    QCoreApplication::setApplicationVersion(QStringLiteral("%1").arg(CuteCom_VERSION));
//    qRegisterMetaType<Settings::LineTerminator>();

    setupUi(this);

    m_bt_sendfile->setEnabled(false);
    m_command_history->setEnabled(false);

    m_lb_logfile->setStyleSheet(" QLabel:hover{color: blue;} ");

    connect(m_bt_clear, &QPushButton::clicked,
            [=](){
                    if(m_device->isOpen())
                        m_input_edit->setFocus();
                    m_output_display->clear();
                    m_hexBytes = 0;
                });
    m_output_font = QFont("Monospace");
    m_output_font.setStyleHint(QFont::Monospace);
    connect(m_check_hex_out, &QCheckBox::toggled, this, &MainWindow::setHexOutput);

    // initialize settings stored in the config file
    m_settings = new Settings(this);
    m_settings->readSettings(session);
    this->setWindowTitle("CuteCom - " + session);

    // restore window size from the settings
    QRect geometry = m_settings->getWindowGeometry();
    if(geometry.width()>0)
        setGeometry(geometry);

    QStringList history = m_settings->getCurrentSession().command_history;
    if (!history.empty())
    {
       m_command_history->addItems(history);
       m_command_history->setCurrentRow(m_command_history->count()-1);
       m_command_history->scrollToItem(m_command_history->currentItem());
       m_command_history->clearSelection();
    }

    fillLineTerminationChooser(m_settings->getLineTerminator());

    fillProtocolChooser(m_settings->getProtocol());

    m_spinner_chardelay->setValue(m_settings->getCharacterDelay());
    connect(m_spinner_chardelay, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged),
            [=](int value){ m_settings->settingChanged(Settings::CharacterDelay, value );});

    // add the settings slide out panel
    controlPanel = new ControlPanel(this->centralWidget(), m_settings);

    QMargins mainMargins = m_verticalLayout->contentsMargins();
    controlPanel->setLeftMargin(mainMargins.left());

    //show the controlPanel
    controlPanel->show();
    // slide this panel to the top
    controlPanel->collapse();

    // make sure there is enough room for the controls
    int minWidth = controlPanel->frameGeometry().width();
    if( minWidth > m_mainSplitter->minimumWidth())
        m_mainSplitter->setMinimumWidth(minWidth);


    // make sure there is enough space for the collapsed panel
    // above all other widgets
    int hHeight = controlPanel->hiddenHeight();
    qDebug() << Q_FUNC_INFO << "calculated height: " << hHeight;
    mainMargins.setTop(hHeight);
    m_verticalLayout->setContentsMargins(mainMargins);
    m_mainSplitter->installEventFilter(this);

    m_device = new QSerialPort(this);

    // setup status bar with initial infromation
    m_device_statusbar = new StatusBar(this);
    m_device_statusbar->sessionChanged(m_settings->getCurrentSession());
    this->statusBar()->addWidget(m_device_statusbar);
    connect(m_settings, &Settings::sessionChanged, m_device_statusbar, &StatusBar::sessionChanged);

    connect(controlPanel, &ControlPanel::openDeviceClicked, this, &MainWindow::openDevice);
    connect(controlPanel, &ControlPanel::closeDeviceClicked, this, &MainWindow::closeDevice);
    connect(m_device, static_cast<void (QSerialPort::*)(QSerialPort::SerialPortError serialPortError)>(&QSerialPort::error), this, &MainWindow::handleError);
    connect(m_device, &QSerialPort::readyRead, this, &MainWindow::displayData );

    m_input_edit->installEventFilter(this);
    connect(&m_keyRepeatTimer, &QTimer::timeout, this, &MainWindow::sendKey );
    connect(m_input_edit, &QLineEdit::returnPressed, this, &MainWindow::execCmd );
    connect(m_command_history, &QListWidget::itemClicked, this, &MainWindow::commandFromHistoryClicked);
    connect(m_command_history, &QListWidget::doubleClicked, this, &MainWindow::execCmd);
    connect(m_bt_sendfile, &QPushButton::clicked, this, &MainWindow::sendFile);

    // tie the control panel's edit and this window's information label together
    m_lb_logfile->setText(m_settings->getLogFileLocation());
    m_lb_logfile->installEventFilter(this);
    controlPanel->m_logfile_edit->setText(m_settings->getLogFileLocation());
    connect(controlPanel->m_logfile_edit, &QLineEdit::editingFinished,
            [=]() {
                    const QString text = controlPanel->m_logfile_edit->text();
                    m_lb_logfile->setText(text);
                    m_settings->settingChanged(Settings::LogFileLocation, text);
                });
    connect(m_check_logging, &QCheckBox::toggled, this, &MainWindow::toggleLogging);

    connect(actionAbout_CuteCom, &QAction::triggered, this, &MainWindow::showAboutMsg);
    connect(actionAbout_Qt, &QAction::triggered, &QApplication::aboutQt);
}

bool MainWindow::eventFilter(QObject * obj, QEvent * event)
{
    if(obj == m_mainSplitter) {
        if( event->type() == QEvent::Resize) {
            if( ((QResizeEvent*) event)->oldSize().width() != m_mainSplitter->width()) {
            //qDebug() << ((QResizeEvent*) event)->oldSize().width() << " : " << m_mainSplitter->width();
                controlPanel->resize(m_verticalLayout->contentsRect().width(), controlPanel->height());
            }
        }
        return false;
    } else if (obj == m_input_edit && event->type() == QEvent::KeyPress) {
        QKeyEvent *ke=(QKeyEvent*)event;
        if (ke->modifiers()==Qt::NoModifier) {
            switch(ke->key()) {
            case Qt::Key_Up :
                prevCmd();
                return true;
            case Qt::Key_Down :
                nextCmd();
                return true;
            case Qt::Key_PageDown : //fall through
            case Qt::Key_PageUp :
                QApplication::sendEvent(m_output_display, event);
                return true;
            default:
                break;
            }
        } else if (ke->modifiers()==Qt::ControlModifier) {
            switch(ke->key()){
            case Qt::Key_C :
                // std::cerr<<"c";
                m_keyCode = 3;
                sendByte(m_keyCode, 0);
                m_keyRepeatTimer.setSingleShot(false);
                m_keyRepeatTimer.start(0);
                return true;
            case Qt::Key_Q :
                // std::cerr<<"#";
                m_keyCode = 17;
                sendByte(m_keyCode, 0);
                return true;
            case Qt::Key_S :
                m_keyCode = 19;
                sendByte(m_keyCode, 0);
                return true;
            case Qt::Key_Down : //fall through
            case Qt::Key_Up : //fall through
            case Qt::Key_PageUp : //fall through
            case Qt::Key_PageDown : //fall through
            case Qt::Key_Home : //fall through
            case Qt::Key_End :
                {
                    QKeyEvent cpy(QEvent::KeyPress, ke->key(), Qt::NoModifier);
                    QApplication::sendEvent(m_output_display, &cpy);
                }
                return true;
            default:
                break;
            }

        }
    } else if ((obj==m_input_edit) && (event->type()==QEvent::KeyRelease)) {
        m_keyRepeatTimer.stop();
        return false;
    } else if ((obj==m_lb_logfile) && (event->type()==QEvent::MouseButtonPress)) {
        if (m_device->isOpen()) {
            QMessageBox::warning(this,tr("Can't change logfile location"),
                                     tr("Device needs to be closed to change the logfile"));
        } else {
            controlPanel->slideOut();
            controlPanel->m_logfile_edit->setFocus();
        }

        return false;
    } else {
        return QMainWindow::eventFilter(obj, event);
    }
    return false;
}

void MainWindow::resizeEvent(QResizeEvent* event)
{
   QMainWindow::resizeEvent(event);
   m_settings->settingChanged(Settings::WindowGeometry, this->frameGeometry());
}

void MainWindow::openDevice()
{
    const Settings::Session session = m_settings->getCurrentSession();
    QMessageBox error;
    error.setIcon( QMessageBox::Critical );
    if(session.device.isEmpty()){        
        error.setText(tr("Opening not possible."));
        error.setInformativeText(tr("No device has been specified"));

        error.exec();
        controlPanel->closeDevice();
        return;
    }
    m_device->setPortName(session.device);
    m_device->setBaudRate(session.baudRate);
    m_device->setDataBits(session.dataBits);
    m_device->setParity(session.parity);
    m_device->setStopBits(session.stopBits);
    m_device->setFlowControl(session.flowControl);

    if (m_device->open(session.openMode)) {
        // display connectin parameter on status bar
        printDeviceInfo();
    }else {
        QMessageBox::critical(this, tr("Opening device not possible"), m_device->errorString());
    }

    if(session.openMode == QIODevice::WriteOnly || session.openMode == QIODevice::ReadWrite) {

        m_device->flush();

        m_input_edit->setEnabled(true);
        m_input_edit->setFocus();
        controlPanel->m_combo_device->setEnabled(false);
        m_bt_sendfile->setEnabled(true);
        m_command_history->setEnabled(true);

        m_hexBytes = 0;
        m_previousChar = '\0';

        m_device_statusbar->setDeviceInfo(m_device);
    }

}

void MainWindow::closeDevice()
{
    m_device->close();
    m_input_edit->setEnabled(false);
    controlPanel->m_bt_open->setFocus();
    controlPanel->m_combo_device->setEnabled(true);
    m_bt_sendfile->setEnabled(false);
    m_command_history->setEnabled(false);
}

void MainWindow::handleError(QSerialPort::SerialPortError error){
    if (error == QSerialPort::ResourceError) {
        QMessageBox::critical(this, tr("Device Error"), m_device->errorString());
        controlPanel->closeDevice();
    }

}

void MainWindow::printDeviceInfo()
{
    QSerialPortInfo info = QSerialPortInfo(*m_device);
    qDebug() << info.description() << info.manufacturer() << info.productIdentifier()
             << info.serialNumber() << info.portName();
    qDebug() << m_device->baudRate() << " : " << m_device->dataBits() << "-"
             << m_device->parity() << "-" << m_device->stopBits() << " # "
             << m_device->flowControl();
}

void MainWindow::toggleLogging(bool start)
{
    if (m_logFile.isOpen()==start)
    {
       return;
    }

    if (start)
    {
       m_logFile.setFileName(m_lb_logfile->text());
       QIODevice::OpenMode mode=QIODevice::ReadWrite;
       mode = (controlPanel->m_check_appendLog->isChecked())? mode | QIODevice::Truncate : mode | QIODevice::Append;
       qDebug() << mode;
       if (!m_logFile.open(mode)) {
           QMessageBox::information(this, tr("Opening file failed"),
                                   tr("Could not open file %1 for writing").arg(m_lb_logfile->text()));
           m_check_logging->setChecked(false);
       }
    } else {
       m_logFile.close();
    }

}

void MainWindow::fillLineTerminationChooser(const Settings::LineTerminator setting)
{
    m_combo_lineterm->addItem(QStringLiteral("LF"), QVariant::fromValue(Settings::LF));
    m_combo_lineterm->addItem(QStringLiteral("CR"), QVariant::fromValue(Settings::CR));
    m_combo_lineterm->addItem(QStringLiteral("CR/LF"), QVariant::fromValue(Settings::CRLF));
    m_combo_lineterm->addItem(tr("None"), QVariant::fromValue(Settings::NONE));
    m_combo_lineterm->addItem(QStringLiteral("Hex"), QVariant::fromValue(Settings::HEX));


    int index = m_combo_lineterm->findData((setting > Settings::HEX)? Settings::LF : setting);
    if(index != -1) {
        m_combo_lineterm->setCurrentIndex(index);
    }
    connect(m_combo_lineterm, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
            [=](){ m_settings->settingChanged(Settings::LineTermination, m_combo_lineterm->currentData()); });
}

/**
 * Fills the ComboBox from which the user can select the protocoll used
 * for sending a file across the device
 * @brief MainWindow::fillProtocolChooser
 * @param setting
 */
void MainWindow::fillProtocolChooser(const Settings::Protocol setting)
{
    m_combo_protocol->addItem(tr("Plain"), QVariant::fromValue(Settings::PLAIN));
    m_combo_protocol->addItem(tr("Script"), QVariant::fromValue(Settings::SCRIPT));
    m_combo_protocol->addItem(tr("XModem"), QVariant::fromValue(Settings::XMODEM));
    m_combo_protocol->addItem(tr("YModem"), QVariant::fromValue(Settings::YMODEM));
    m_combo_protocol->addItem(tr("ZModem"), QVariant::fromValue(Settings::ZMODEM));
    m_combo_protocol->addItem(tr("1KXModem"), QVariant::fromValue(Settings::ONEKXMODEM));

    int index = m_combo_protocol->findData((setting < Settings::PROTOCOL_MAX)? setting : Settings::PLAIN);
    if(index != -1) {
        m_combo_protocol->setCurrentIndex(index);
    }
    connect(m_combo_protocol, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
            [=](){ m_settings->settingChanged(Settings::ProtocolOption, m_combo_protocol->currentData()); });
}

void MainWindow::showAboutMsg()
{
   QMessageBox::about(this, tr("About CuteCom"),
                      tr("This is CuteCom %1<br>(c)2004-2009 Alexander Neundorf, &lt;neundorf@kde.org&gt;"
                          "<br>(c)2015 Meinhard Ritscher, &lt;unreachable@gmx.net&gt;<br> and contributors"
                          "<br>Licensed under the GNU GPL version 3 (or any later version).")
                      .arg(CuteCom_VERSION));
}

void MainWindow::setHexOutput(bool checked)
{
    if(checked) {
        m_output_display->setFont(m_output_font);
    } else {
        // ToDo make this changeable via settings
        QFont defaultFont;
        defaultFont.setFamily(defaultFont.defaultFamily());
        m_output_display->setFont(defaultFont);
    }
}

void MainWindow::prevCmd()
{
    if (m_command_history->count()<=m_cmdBufIndex)
    {
       return;
    }

    m_cmdBufIndex++;
    QListWidgetItem* item=m_command_history->item(m_command_history->count()-m_cmdBufIndex);
    if (item!=0)
    {
       m_command_history->setCurrentItem(item);
       m_input_edit->setText(item->text());
    }

}

void MainWindow::nextCmd()
{
    if (m_cmdBufIndex==0)
    {
       return;
    }

    m_cmdBufIndex--;
    if (m_cmdBufIndex==0)
    {
       m_input_edit->clear();
       m_command_history->setCurrentItem(0);
    }
    else
    {
       QListWidgetItem* it=m_command_history->item(m_command_history->count()-m_cmdBufIndex);
       m_command_history->setCurrentItem(it);
       m_input_edit->setText(it->text());
    }
}

void MainWindow::execCmd()
{
    m_cmdBufIndex = 0;
    QString cmd = m_input_edit->text().trimmed();
    m_input_edit->clear();
    if (!cmd.isEmpty())
    {
       bool found = false;
       QList<QListWidgetItem *> list = m_command_history->findItems(cmd, 0);
       foreach(QListWidgetItem * item, list)
       {
          item = m_command_history->takeItem(m_command_history->row(item));
          delete item;
          found = true;
       }

       m_command_history->addItem(cmd);
       m_command_history->setCurrentRow(m_command_history->count()-1);
       if (m_command_history->count()>50)
           m_command_history->removeItemWidget(m_command_history->item(0));

       // Do not save settings if there is no new string
       if (!found)
          saveCommandHistory();
       // emit settingChanged .... // emit commandHistoryChanged and connect to settings slot above
    }
    m_command_history->clearSelection();
    if (!m_device->isOpen())
    {
       return;
    }

    sendString(cmd);

}

void MainWindow::commandFromHistoryClicked(QListWidgetItem *item)
{
    if(item == 0)
        return;
    m_input_edit->setText(item->text());
    m_input_edit->setFocus();
}

void MainWindow::saveCommandHistory()
{
    QStringList history;
    for (int i=0; i<m_command_history->count(); i++)
    {
       history << m_command_history->item(i)->text();
    }
    m_settings->settingChanged(Settings::CommandHistory, history);
}

bool MainWindow::sendString(const QString& s)
{
    Settings::LineTerminator lineMode= m_combo_lineterm->currentData().value<Settings::LineTerminator>();
    //ToDo
    unsigned int charDelay = m_spinner_chardelay->value();

    if (lineMode==Settings::HEX) // hex
    {
       QString hex=s;
       hex.remove(QRegExp("\\s"));
       if ((hex.startsWith("0x")) || (hex.startsWith("0X")))
       {
          hex=hex.mid(2);
       }

       if (hex.length()%2 != 0)
       {
          hex="0"+hex;
       }

       for (int i=0; i<hex.length()/2; i++)
       {
          QString nextByte=hex.mid(i*2, 2);
          bool ok=true;
          nextByte.toUInt(&ok, 16);
          if (!ok)
          {
             QMessageBox::information(this, tr("Invalid hex characters"), tr("The input string contains invalid hex characters: 0x%1").arg(nextByte));
             return false;
          }
       }

       for (int i=0; i<hex.length()/2; i++)
       {
          QString nextByte=hex.mid(i*2, 2);
          unsigned int byte=nextByte.toUInt(0, 16);
          sendByte(byte & 0xff, charDelay);
          // fprintf(stderr, " 0x%x d:%d ", byte & 0xff, charDelay);
       }
       return true;
    }

    const QByteArray bytes = s.toLatin1();
    for (int i=0; i<bytes.length(); i++)
    {
       if (!sendByte(bytes[i], charDelay))
          return false;
    }

    switch (lineMode) {
    case Settings::LF:
        if (!sendByte('\n', charDelay))
           return false;
        break;
    case Settings::CR:
        if (!sendByte('\r', charDelay))
           return false;
        break;
    case Settings::CRLF:
        if (!sendByte('\r', charDelay))
           return false;
        if (!sendByte('\n', charDelay))
           return false;
        break;
    default:
        break;
    }

    return true;
}


bool MainWindow::sendByte(const char c, unsigned long delay)
{
    if(!m_device->isOpen()) {
        return false;
    }
    if((m_device->write(&c, 1)) < 1){
        qDebug() << m_device->errorString();
        return false;
    }

    if(delay) {
        millisleep(delay);
        m_device->flush();
    }
    return true;
}

void MainWindow::sendKey()
{
    sendByte(m_keyCode, 0);
}

/**
 * Presents a file chooser dialog with which the user may select one
 * single file which will be sent across the previously opened serial port
 * device using the protocol selected
 * @brief MainWindow::sendFile
 */
void MainWindow::sendFile()
{
    QFileDialog fileDlg(this);

    fileDlg.setDirectory(m_settings->getSendStartDir());
    fileDlg.setFileMode(QFileDialog::ExistingFile);

    QString filename;
    if ( fileDlg.exec() ) {
        QStringList fn = fileDlg.selectedFiles();
        if(!fn.isEmpty())
           filename = fn[0];
        else
            return;
        m_settings->settingChanged(Settings::SendStartDir, fileDlg.directory().absolutePath());
    } else {
       return;
    }

    unsigned long charDelay=static_cast<unsigned long>(m_spinner_chardelay->value());

    Settings::Protocol protocol = m_combo_protocol->currentData().value<Settings::Protocol>();
    if(protocol == Settings::PLAIN)
    {
        QFile fd(filename);
        if(!fd.open(QIODevice::ReadOnly)) {
            QMessageBox::warning(this, tr("Opening file failed"), tr("Could not open file %1").arg(filename));
            return;
        }
        QByteArray data = fd.readAll();
        delete m_progress;
        m_progress = new QProgressDialog(tr("Sending file..."), tr("Cancel"), 0, 100, this);
        m_progress->setMinimumDuration(100);
        unsigned int step = data.size()/100;
        if(step < 1) {
            step = 1;
        }
        for(int i=0; i<data.size(); i++) {
            if((i % step) == 0) {
                m_progress->setValue(i/step);
                qApp->processEvents();
            }
            sendByte(data.data()[i], charDelay);
            if((data.data()[i]=='\n') || (data.data()[i]='\r')) {
                //waiting twice as long after bytes whigh might by line ends
                //(this helps some uCs)
                millisleep(charDelay);
            }
            if(0) {
                QMessageBox::information(this, tr("Comm error"), tr("Sending failed (%1/%2").arg(i).arg(data.count()));
                break;
            }
            if( m_progress->wasCanceled())
                break;

        }
        delete m_progress;
        m_progress = 0;


    } else if (protocol == Settings::SCRIPT) {
        QFile fd(filename);
        if(!fd.open(QIODevice::ReadOnly)) {
            QMessageBox::warning(this, tr("Opening file failed"), tr("Could not open file %1").arg(filename));
            return;
        }

        QTextStream stream(&fd);
        while(!stream.atEnd()) {
            QString nextLine = stream.readLine();
            nextLine = nextLine.left((unsigned int) nextLine.indexOf("#"));
            if(nextLine.isEmpty())
                continue;

            if(!sendString(nextLine)) {
                QMessageBox::information(this, tr("Comm error"), tr("Sending failed"));
                return;
            }
            millisleep(charDelay * 3);
        }

    } else if (protocol == Settings::XMODEM
               || protocol == Settings::YMODEM
               || protocol == Settings::ZMODEM
               || protocol == Settings::ONEKXMODEM) {
        closeDevice();
        m_sz = new QProcess(this);

        QStringList listProcessArgs;
        listProcessArgs.append("-c");

        QString tmp = QString("sz ");
        switch(protocol) {
        case Settings::XMODEM:
            tmp += "--xmodem";
            break;
        case Settings::YMODEM:
            tmp += "--ymodem";
            break;
        case Settings::ZMODEM:
            tmp += "--zmodem";
            break;
        case Settings::ONEKXMODEM:
            tmp += "--xmodem --1k";
            break;
        default:
            break;
        }
        QString device = controlPanel->m_combo_device->currentText();
        tmp = tmp + "-vv \"" + filename + "\" < " + device + " > " + device;
        listProcessArgs.append(tmp);

        m_sz->setReadChannel(QProcess::StandardError);

        connect(m_sz, &QProcess::readyReadStandardError, this, &MainWindow::readFromStdErr);
        connect(m_sz, static_cast<void (QProcess::*)(int, QProcess::ExitStatus)>(&QProcess::finished), this, &MainWindow::sendDone);

        m_sz->start("sh", listProcessArgs);
        if(!!m_sz->waitForStarted()) {
            QMessageBox::information(this, tr("Comm error"), tr("Could not start sz"));
            delete m_sz;
            openDevice();
            return;
        }
        m_progress = new QProgressDialog(tr("Sending file via %1 ...").arg(protocol), tr("Cancel"), 0, 100, this);
        connect(m_progress, &QProgressDialog::canceled, this, &MainWindow::killSz);
        m_progress->setMinimumDuration(100);
        QFileInfo fi(filename);
        m_progressStepSize = fi.size()/1024/100;
        if(m_progressStepSize < 1)
            m_progressStepSize = 1;

        m_progress->setValue(0);
        while(m_sz->state() == QProcess::Running) {
            qApp->processEvents();
            millisleep(10L);
        }

        delete m_sz;
        m_sz=0;
        delete m_progress;
        m_progress = 0;
        openDevice();
    } else {
        QMessageBox::information(this, tr("Unsupported Protocol"), tr("The selected protocoll is not supported (yet)"));
    }

}

/**
 * @brief MainWindow::killSz
 */
void MainWindow::killSz()
{
    if(m_sz==0)
        return;
    m_sz->terminate();
}

/**
 * THIS IS NO LONGER USED?????
 * redirects the output from the Sz process
 * to the serial port device
 * @brief MainWindow::readFromStdOut
 */
void MainWindow::readFromStdOut()
{
    QByteArray data = m_sz->readAllStandardOutput();

    quint64 bytesToWrite = data.size();
    qint64 bytesWritten = 0;
    while(bytesToWrite > 0 ){
        bytesWritten = m_device->write(data);
        if(bytesWritten < 0) {
            // qDebug() << "error writing to device;
            return;
        }
        bytesToWrite -= bytesWritten;
        data = data.right(bytesToWrite);
    }
}

/**
 * @brief MainWindow::readFromStdErr
 */
void MainWindow::readFromStdErr()
{
    QByteArray ba=m_sz->readAllStandardError();
    if (m_progress==0)
    {
       return;
    }
    QString s(ba);
    QRegExp regex(".+\\d+/ *(\\d+)k.*");
    int pos=regex.indexIn(s);
    if (pos>-1)
    {
       QString captured=regex.cap(1);
 //      cerr<<"captured kb: -"<<captured.latin1()<<"-"<<std::endl;
       int kb=captured.toUInt();
       if ((kb%m_progressStepSize)==0)
       {
          int p=kb/m_progressStepSize;
          if (p<100)
          {
             m_progress->setValue(p);
          }
       }
    }
 //   else
 //      cerr<<"--------"<<s.latin1()<<"-"<<std::endl;
 /*   for (unsigned int i=0; i<ba.count(); i++)
    {
       char c=ba.data()[i];
       unsigned int tmp=c;
       if (isprint(tmp))
          cerr<<c;
       else
          fprintf(stderr, " \\0x%02x ", tmp&0xff);
    }*/
}

/**
 * IS THIS REALLY NEEDED?????
 * @brief MainWindow::sendDone
 * @param exitCode
 * @param exitStatus
 */
void MainWindow::sendDone(int exitCode,
                               QProcess::ExitStatus exitStatus)
{
    qDebug() <<"sx exited with: " << exitCode << exitStatus;
}

/**
 * @todo research optimization of the conversion to Hex
 * https://forum.qt.io/topic/25145/solved-qbytearray-values-to-hex-formatted-qstring/5
 * @brief MainWindow::displayData
 */
void MainWindow::displayData()
{
    QByteArray data = m_device->readAll();
    // Debugging:
    //QString temp = QString(QStringLiteral("abcd\ncd\tef\nuvwxyz12345\r\n67890123456\r\n-----\2---\0---\n"));
    //QByteArray data = temp.toLatin1();
    // :Debugging

    if (m_logFile.isOpen())
    {
       m_logFile.write(data);
    }

    QString formattedString;

    char buf[16];
    for (int i=0; i<data.size(); i++)
    {
       unsigned int b=data.at(i);

       if (m_check_hex_out->isChecked())
       {
          static QString text_ascii;
          if ((m_hexBytes % 16) == 0)
          {
             snprintf(buf, 16, "%08x: ", m_hexBytes);
             formattedString+=buf;
             text_ascii='\t';
          }

          snprintf(buf, 16, "%02x ", b & 0xff);
          formattedString+=buf;
          if (b < 0x20)
              b += 0x2400;
          else if (0x7F <= b)
              b = '.';
              text_ascii += QChar(b);

          m_hexBytes++;
          if ((m_hexBytes % 16)==0)
          {
              formattedString += text_ascii;
              formattedString += "\n";
          }
          else if ((m_hexBytes % 8)==0)
          {
              formattedString += "  ";
              text_ascii += "  ";
          }
       }
       else
       {
          static bool state_ctrl = false;

          // also print a newline for \r, and print only one newline for \r\n
          if ((isprint(b)) || (b=='\n') || (b=='\r') || (b=='\t'))
          {
              // a terminating '\0' was detected, start on a new line
             if (state_ctrl) {
               formattedString+='\n';
               state_ctrl = false;
             }

             if (b=='\r')
             {
                 if (controlPanel->m_check_lineBreak->isChecked())
                    formattedString+=QChar(0x240D);
             }
             else if (b=='\n')
             {
                if (controlPanel->m_check_lineBreak->isChecked())
                    formattedString+=QChar(0x240A);
                formattedString+='\n';
             }
             else if (b=='\t')
             {
                 if (controlPanel->m_check_lineBreak->isChecked())
                     formattedString+=QChar(0x21E5);
                 formattedString+='\t';
             }
             else
             {
                formattedString+=b;
             }

             m_previousChar = b;
          }
          else
          {
              state_ctrl = true;
              if (b == '\0')
              {
                 formattedString+="<break>";

              } else {
                 snprintf(buf, 16, "<0x%02x>", b & 0xff);
                 formattedString+=buf;
              }
          }
       }
    }

    if (controlPanel->m_check_timestamp->isChecked()) {
        m_timestamp = QTime::currentTime();
        QString timestring = QStringLiteral("\n[")
                + m_timestamp.toString(QStringLiteral("HH:mm:ss:zzz"))
                + QStringLiteral("]");
        formattedString.replace("\n", timestring);
    }

    QScrollBar *sb = m_output_display->verticalScrollBar();
    int save_scroll = sb->value();
    int save_max = (save_scroll == sb->maximum());

    m_output_display->moveCursor( QTextCursor::End);
    m_output_display->insertPlainText(formattedString);

    if(save_max)
        sb->setValue(sb->maximum());
    else
        sb->setValue(save_max);
}


MainWindow::~MainWindow()
{

    if(m_device->isOpen())
        m_device->close();
    delete m_settings;
}
