/*  Copyright (C) 2004-2005 Alexander Neundorf <neundorf@kde.org>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/

#ifndef QCPPIALOGIMPL_H
#define QCPPIALOGIMPL_H

#include "ui_cutecommdlg.h"

#include <termios.h>

#include <qsocketnotifier.h>
#include <qtimer.h>
#include <qdatetime.h>
#include <qfile.h>
//Added by qt3to4:
#include <QResizeEvent>
#include <QEvent>
#include <QWidget>

#define CUTECOMM_BUFSIZE (4096)

class QListWidgetItem;
class QResizeEvent;
class Q3Process;
class QProgressDialog;
class QFileDialog;

class QCPPDialogImpl:public QWidget, public Ui::CuteCommDlg
{
   Q_OBJECT
   public:
      QCPPDialogImpl(QWidget* parent);
      virtual bool eventFilter(QObject* watched, QEvent *e);
   protected slots:
      void execCmd();
      void readData(int fd);
      void sendFile();
      void showAboutMsg();

      void oldCmdClicked(QListWidgetItem* item);
      void saveSettings();
      void readFromStdout();
      void readFromStderr();
      void sendDone();
      void connectTTY();
      void disconnectTTY();
      void killSz();
      void enableSettingWidgets(bool enable);
      void doOutput();
      void sendKey();
      void hexOutputClicked(bool on);
      void enableLogging(bool on);
      void chooseLogFile();
      void clearOutput();
   protected:
      void fillBaudCb();
      void addOutput(const QString& text);
      bool sendByte(char c, unsigned int delay);
      void disconnectTTYRestore(bool restore);
      void readSettings();
      void prevCmd();
      void nextCmd();
      bool sendString(const QString& s);
      void setNewOptions(int baudrate, int databits, const QString& parity, const QString& stop, bool softwareHandshake, bool hardwareHandshake);
      virtual void resizeEvent(QResizeEvent *e);

      bool m_isConnected;
      int m_fd;
      struct termios m_oldtio;
      unsigned int m_cmdBufIndex;
      QSocketNotifier *m_notifier;
      char m_buf[CUTECOMM_BUFSIZE];
      Q3Process *m_sz;
      QProgressDialog *m_progress;
      int m_progressStepSize;

      QFileDialog *m_fileDlg;
      QString m_sendFileDialogStartDir;

      QTimer m_outputTimer;
      QTime m_outputTimerStart;
      QString m_outputBuffer;

      QTimer m_keyRepeatTimer;
      char m_keyCode;
      unsigned int m_hexBytes;
      char m_previousChar;

      QFile m_logFile;

};

#endif
