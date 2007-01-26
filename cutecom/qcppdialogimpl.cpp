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

#include "qcppdialogimpl.h"

#include <qcombobox.h>
#include <qpushbutton.h>
#include <qcheckbox.h>
#include <qtextbrowser.h>
#include <qlistbox.h>
#include <qlineedit.h>
#include <qfiledialog.h>
#include <qdir.h>
#include <qfile.h>
#include <qmessagebox.h>
#include <qsettings.h>
#include <qevent.h>
#include <qcstring.h>
#include <qprogressdialog.h>
#include <qapplication.h>
#include <qprocess.h>
#include <qfileinfo.h>
#include <qregexp.h>
#include <qspinbox.h>

#include <iostream>
using namespace std;

#include <errno.h>
#include <unistd.h>
#include <ctype.h>
#include <sys/ioctl.h>
#include <sys/termios.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/select.h>
#include <fcntl.h>

void millisleep(int ms)
{
   if (ms>0)
   {
      struct timeval tv;
      tv.tv_sec=0;
      tv.tv_usec=ms*1000;
      select(0, 0, 0, 0, &tv);
   }
}

QCPPDialogImpl::QCPPDialogImpl(QWidget* parent)
:CuteCommDlg(parent)
,m_isConnected(false)
,m_fd(-1)
,m_cmdBufIndex(0)
,m_notifier(0)
,m_sz(0)
,m_progress(0)
,m_progressStepSize(1000)
,m_fileDlg(0)
,m_outputTimer(this)
,m_keyRepeatTimer(this)
,m_keyCode(0)
//,m_firstRep(true)
,m_hexBytes(0)
{
   connect(m_connectPb, SIGNAL(clicked()), this, SLOT(connectTTY()));
   connect(m_closePb, SIGNAL(clicked()), this, SLOT(disconnectTTY()));
   connect(m_clearOutputPb, SIGNAL(clicked()), m_outputView, SLOT(clear()));
//   connect(m_clearInputPb, SIGNAL(clicked()), m_oldCmdsLb, SLOT(clear()));

   connect(m_cmdLe, SIGNAL(returnPressed()), this, SLOT(execCmd()));

   connect(m_sendPb, SIGNAL(clicked()), this, SLOT(sendFile()));
   connect(m_aboutPb, SIGNAL(clicked()), this, SLOT(showAboutMsg()));
   connect(m_quitPb, SIGNAL(clicked()), this, SLOT(close()));

   connect(m_oldCmdsLb, SIGNAL(clicked(QListBoxItem*)), this, SLOT(oldCmdClicked(QListBoxItem*)));
   connect(m_oldCmdsLb, SIGNAL(doubleClicked(QListBoxItem*)), this, SLOT(execCmd()));

   connect(m_hexOutputCb, SIGNAL(toggled(bool)), this, SLOT(hexOutputClicked(bool)));

   connect(m_connectPb, SIGNAL(clicked()), this, SLOT(saveSettings()));
//   connect(m_deviceCb, SIGNAL(activated(int)), this, SLOT(saveSettings()));
   connect(m_baudCb, SIGNAL(activated(int)), this, SLOT(saveSettings()));
   connect(m_dataBitsCb, SIGNAL(activated(int)), this, SLOT(saveSettings()));
   connect(m_parityCb, SIGNAL(activated(int)), this, SLOT(saveSettings()));
   connect(m_stopCb, SIGNAL(activated(int)), this, SLOT(saveSettings()));
   connect(m_protoPb, SIGNAL(activated(int)), this, SLOT(saveSettings()));
   connect(m_softwareCb, SIGNAL(clicked()), this, SLOT(saveSettings()));
   connect(m_hardwareCb, SIGNAL(clicked()), this, SLOT(saveSettings()));
   connect(m_readCb, SIGNAL(clicked()), this, SLOT(saveSettings()));
   connect(m_writeCb, SIGNAL(clicked()), this, SLOT(saveSettings()));
   connect(m_applyCb, SIGNAL(clicked()), this, SLOT(saveSettings()));
   connect(m_hexOutputCb, SIGNAL(clicked()), this, SLOT(saveSettings()));
   connect(m_inputModeCb, SIGNAL(activated(int)), this, SLOT(saveSettings()));
   connect(m_charDelaySb, SIGNAL(valueChanged(int)), this, SLOT(saveSettings()));
   connect(m_logAppendCb, SIGNAL(activated(int)), this, SLOT(saveSettings()));

   connect(m_applyCb, SIGNAL(toggled(bool)), this, SLOT(enableSettingWidgets(bool)));
   connect(m_logFileFileDialog, SIGNAL(clicked()), this, SLOT(chooseLogFile()));

   connect(&m_outputTimer, SIGNAL(timeout()), this, SLOT(doOutput()));
   connect(&m_keyRepeatTimer, SIGNAL(timeout()), this, SLOT(sendKey()));

   connect(m_enableLoggingCb, SIGNAL(toggled(bool)), this, SLOT(enableLogging(bool)));
//   connect(m_enableLoggingCb, SIGNAL(toggled(bool)), this, SLOT(enableLogging(bool)));

   m_outputView->setWrapPolicy(QTextEdit::Anywhere);
   m_outputView->setWordWrap(QTextEdit::WidgetWidth);

/*   QAccel* accel=new QAccel(this);
   accel->insertItem(CTRL+Key_C, 3);
   accel->insertItem(CTRL+Key_Q, 17);
   accel->insertItem(CTRL+Key_S, 19);
   connect(accel, SIGNAL(activated(int)), this, SLOT(sendByte(int)));*/

   m_outputTimerStart.start();

   readSettings();

   disconnectTTY();

   m_cmdLe->installEventFilter(this);
}

void QCPPDialogImpl::resizeEvent(QResizeEvent *e)
{
   QWidget::resizeEvent(e);
   saveSettings();
}

void QCPPDialogImpl::saveSettings()
{
   QSettings settings;
   settings.writeEntry("/cutecom/HardwareHandshake", m_hardwareCb->isChecked());
   settings.writeEntry("/cutecom/SoftwareHandshake", m_softwareCb->isChecked());

   settings.writeEntry("/cutecom/OpenForReading", m_readCb->isChecked());
   settings.writeEntry("/cutecom/OpenForWriting", m_writeCb->isChecked());
   settings.writeEntry("/cutecom/DontApplySettings", !m_applyCb->isChecked());

   //   settings.writeEntry("/cutecom/Device", m_deviceCb->currentItem());
   settings.writeEntry("/cutecom/Baud", m_baudCb->currentItem());
   settings.writeEntry("/cutecom/Databits", m_dataBitsCb->currentItem());
   settings.writeEntry("/cutecom/Parity", m_parityCb->currentItem());
   settings.writeEntry("/cutecom/Stopbits", m_stopCb->currentItem());
   settings.writeEntry("/cutecom/Protocol", m_protoPb->currentItem());
   settings.writeEntry("/cutecom/width", width());
   settings.writeEntry("/cutecom/height", height());

   settings.writeEntry("/cutecom/LineMode", m_inputModeCb->currentItem());
   settings.writeEntry("/cutecom/HexOutput", m_hexOutputCb->isChecked());
   settings.writeEntry("/cutecom/CharDelay", m_charDelaySb->value());

   settings.writeEntry("/cutecom/SendFileDialogStartDir", m_sendFileDialogStartDir);
   settings.writeEntry("/cutecom/LogFileName", m_logFileLe->text());

   settings.writeEntry("/cutecom/AppendToLogFile", m_logAppendCb->currentItem());


   QString currentDevice=m_deviceCb->currentText();
   settings.writeEntry("/cutecom/CurrentDevice", currentDevice);
   bool currentDeviceIsInList=false;
   QStringList devices;
   for (int i=0; i<m_deviceCb->count(); i++)
   {
      QString s=m_deviceCb->text(i);
      devices<<s;
      if (s==currentDevice)
         currentDeviceIsInList=true;
   }

   if (!currentDeviceIsInList)
      devices<<currentDevice;

   settings.writeEntry("/cutecom/AllDevices", devices);

   int historyCount=m_oldCmdsLb->count();
   if (historyCount>50)
      historyCount=50;

   QStringList saveHist;
   for (unsigned int i=m_oldCmdsLb->count()-historyCount; i<m_oldCmdsLb->count(); i++)
      saveHist<< m_oldCmdsLb->text(i);
   settings.writeEntry("/cutecom/History", saveHist);
}

void QCPPDialogImpl::readSettings()
{
   QSettings settings;
   m_hardwareCb->setChecked(settings.readBoolEntry("/cutecom/HardwareHandshake", false));
   m_softwareCb->setChecked(settings.readBoolEntry("/cutecom/SoftwareHandshake", false));
   m_readCb->setChecked(settings.readBoolEntry("/cutecom/OpenForReading", true));
   m_writeCb->setChecked(settings.readBoolEntry("/cutecom/OpenForWriting", true));

   m_applyCb->setChecked(!settings.readBoolEntry("/cutecom/DontApplySettings", false));
   enableSettingWidgets(m_applyCb->isChecked());

   m_baudCb->setCurrentItem(settings.readNumEntry("/cutecom/Baud", 7));
   m_dataBitsCb->setCurrentItem(settings.readNumEntry("/cutecom/Databits", 3));
   m_parityCb->setCurrentItem(settings.readNumEntry("/cutecom/Parity", 0));
   m_stopCb->setCurrentItem(settings.readNumEntry("/cutecom/Stopbits", 0));
   m_protoPb->setCurrentItem(settings.readNumEntry("/cutecom/Protocol", 0));

   m_inputModeCb->setCurrentItem(settings.readNumEntry("/cutecom/LineMode", 0));
   m_hexOutputCb->setChecked(settings.readBoolEntry("/cutecom/HexOutput", false));
   m_charDelaySb->setValue(settings.readNumEntry("/cutecom/CharDelay", 1));

   m_sendFileDialogStartDir=settings.readEntry("/cutecom/SendFileDialogStartDir", QDir::homeDirPath());
   m_logFileLe->setText(settings.readEntry("/cutecom/LogFileName", QDir::homeDirPath()+"/cutecom.log"));

   m_logAppendCb->setCurrentItem(settings.readNumEntry("/cutecom/AppendToLogFile", 0));

   int x=settings.readNumEntry("/cutecom/width", -1);
   int y=settings.readNumEntry("/cutecom/height", -1);
   if ((x>100) && (y>100))
      resize(x,y);

   bool entryFound=false;
   QStringList devices=settings.readListEntry("/cutecom/AllDevices", &entryFound);
   if (!entryFound)
      devices<<"/dev/ttyS0"<<"/dev/ttyS1"<<"/dev/ttyS2"<<"/dev/ttyS3";

   m_deviceCb->insertStringList(devices);

   m_deviceCb->setCurrentText(settings.readEntry("/cutecom/CurrentDevice", "/dev/ttyS0"));

   QStringList history=settings.readListEntry("/cutecom/History");

   if (!history.empty())
   {
      m_oldCmdsLb->insertStringList(history);
      m_oldCmdsLb->setCurrentItem(m_oldCmdsLb->count()-1);
      m_oldCmdsLb->ensureCurrentVisible();
      m_oldCmdsLb->clearSelection();
   }
}

void QCPPDialogImpl::showAboutMsg()
{
   QMessageBox::about(this, tr("About CuteCom"), tr("This is CuteCom 0.14.2<br>(c)2004-2006 Alexander Neundorf, &lt;neundorf@kde.org&gt;<br>Licensed under the GNU GPL v2"));
}

void QCPPDialogImpl::sendFile()
{
   if (m_fileDlg==0)
   {
      m_fileDlg=new QFileDialog(m_sendFileDialogStartDir);
      m_fileDlg->setMode(QFileDialog::ExistingFile);
   }

   QString filename;
   if ( m_fileDlg->exec() == QDialog::Accepted )
   {
      filename = m_fileDlg->selectedFile();
      m_sendFileDialogStartDir=m_fileDlg->dirPath();
      saveSettings();
   }
   else
   {
      return;
   }

   unsigned int charDelay=m_charDelaySb->value();
   if (m_protoPb->currentText()=="Script")
   {
      QFile file(filename);
      if (!file.open(IO_ReadOnly))
      {
         QMessageBox::information(this, tr("Opening file failed"), tr("Could not open file %1").arg(filename));
         return;
      }

      QTextStream stream(&file);
      while (!stream.atEnd())
      {
         QString nextLine=stream.readLine();
         nextLine=nextLine.left((unsigned int)nextLine.find('#'));
         if (nextLine.isEmpty())
            continue;

         if (!sendString(nextLine))
         {
            QMessageBox::information(this, tr("Comm error"), tr("Sending failed"));
            return;
         }
         millisleep(charDelay*3);
      }
   }
   else if (m_protoPb->currentText()=="Plain")
   {
      QFile file(filename);
      if (!file.open(IO_ReadOnly))
      {
         QMessageBox::information(this, tr("Opening file failed"), tr("Could not open file %1").arg(filename));
         return;
      }
      QByteArray data=file.readAll();
      delete m_progress;
      m_progress=new QProgressDialog(tr("Sending file..."), tr("Cancel"), 100, this, "progress", TRUE);
      m_progress->setMinimumDuration(100);
      unsigned int step=data.size()/100;
      if (step<1)
         step=1;
      for (unsigned int i=0; i<data.size(); i++)
      {
         if ((i%step)==0)
         {
            m_progress->setProgress(i/step);
            qApp->processEvents();
         }
         sendByte(data.data()[i], charDelay);
         if ((data.data()[i]=='\n') || (data.data()[i]=='\r')) //wait twice as long after bytes which might be line ends (helps some uCs)
            millisleep(charDelay);
//         if (!sendByte(data.data()[i]))
         if (0)
         {
            QMessageBox::information(this, tr("Comm error"), tr("Sending failed (%1/%2").arg(i).arg(data.count()));
            break;
         }
         if ( m_progress->wasCancelled() )
            break;
      }
      delete m_progress;
      m_progress=0;
   }
   else if ((m_protoPb->currentText()=="XModem")
            || (m_protoPb->currentText()=="YModem")
            || (m_protoPb->currentText()=="ZModem")
            || (m_protoPb->currentText()=="1kXModem"))
   {
//      QProcess sx(this);
      disconnectTTYRestore(false);
      m_sz=new QProcess(this);
      m_sz->addArgument("sh");
      m_sz->addArgument("-c");
//      QString tmp=QString("sx -vv \"")+filename+"\" < "+m_deviceCb->currentText()+" > "+m_deviceCb->currentText();
      QString tmp=QString("sz ");
      if (m_protoPb->currentText()=="XModem")
         tmp+="--xmodem ";
      else if (m_protoPb->currentText()=="YModem")
         tmp+="--ymodem ";
      else if (m_protoPb->currentText()=="ZModem")
         tmp+="--zmodem ";
      else if (m_protoPb->currentText()=="1kXModem")
         tmp+="--xmodem --1k ";
      
      tmp=tmp+"-vv \""+filename+"\" < "+m_deviceCb->currentText()+" > "+m_deviceCb->currentText();
      m_sz->addArgument(tmp);
      m_sz->setCommunication(QProcess::Stderr);

      connect(m_sz, SIGNAL(readyReadStderr()), this, SLOT(readFromStderr()));
/*      m_sz->addArgument("sx");
      m_sz->addArgument("-vv");
      m_sz->addArgument(filename);
      m_sz->setCommunication(QProcess::Stdin|QProcess::Stdout|QProcess::Stderr);

      connect(m_sz, SIGNAL(readyReadStdout()), this, SLOT(readFromStdout()));
      connect(m_sz, SIGNAL(readyReadStderr()), this, SLOT(readFromStderr()));*/
      connect(m_sz, SIGNAL(processExited()), this, SLOT(sendDone()));
      if (!m_sz->start())
      {
         QMessageBox::information(this, tr("Comm error"), tr("Could not start sx"));
         delete m_sz;
         m_sz=0;
         connectTTY();
         return;
      }
      m_progress=new QProgressDialog(tr("Sending file via xmodem..."), tr("Cancel"), 100, this, "progress", TRUE);
      connect(m_progress, SIGNAL(cancelled()), this, SLOT(killSz()));
      m_progress->setMinimumDuration(100);
      QFileInfo fi(filename);
      m_progressStepSize=fi.size()/1024/100;
      if (m_progressStepSize<1)
         m_progressStepSize=1;
//    cerr<<"while(isRunning)"<<endl;
      m_progress->setProgress(0);
      while (m_sz->isRunning())
      {
         qApp->processEvents();
         millisleep(10);
      }
//      cerr<<"----------------- sx done"<<endl;

      delete m_sz;
      m_sz=0;
      delete m_progress;
      m_progress=0;
      connectTTY();
   }
   else
   {
      QMessageBox::information(this, tr("Opening file failed"), tr("Protocol %1 not supported yet").arg(m_protoPb->currentText()));
   }
}

void QCPPDialogImpl::killSz()
{
   if (m_sz==0)
      return;
   m_sz->tryTerminate();
}

void QCPPDialogImpl::readFromStdout()
{
   QByteArray ba=m_sz->readStdout();
//   cerr<<"readFromStdout() "<<ba.count()<<endl;
   unsigned int bytesToWrite=ba.count();
   char* src=ba.data();
   while (bytesToWrite>0)
   {
      int bytesWritten=::write(m_fd, src, (bytesToWrite>CUTECOMM_BUFSIZE?CUTECOMM_BUFSIZE:bytesToWrite));
      if (bytesWritten<0)
      {
//         cerr<<"readFromStdout() error "<<bytesWritten<<" , "<<bytesToWrite<<" left"<<endl;
         return;
      }
      src+=bytesWritten;
      bytesToWrite-=bytesWritten;

   }
}

void QCPPDialogImpl::readFromStderr()
{
   QByteArray ba=m_sz->readStderr();
//   cerr<<"readFromStderr() "<<ba.count()<<endl;
   if (m_progress==0)
      return;
   QString s(ba);
   QRegExp regex(".+\\d+/ *(\\d+)k.*");
   int pos=regex.search(s);
   if (pos>-1)
   {
      QString captured=regex.cap(1);
//      cerr<<"captured kb: -"<<captured.latin1()<<"-"<<endl;
      int kb=captured.toUInt();
      if ((kb%m_progressStepSize)==0)
      {
         int p=kb/m_progressStepSize;
         if (p<100)
            m_progress->setProgress(p);
      }
   }
//   else
//      cerr<<"--------"<<s.latin1()<<"-"<<endl;
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

void QCPPDialogImpl::sendDone()
{
   cerr<<"sx exited"<<endl;
}

bool QCPPDialogImpl::eventFilter(QObject* watched, QEvent *e)
{
   QKeyEvent *ke=(QKeyEvent*)e;
   if ((watched==m_cmdLe)
       && (e->type()==QEvent::KeyPress))
   {
      if (ke->state()==0)
      {
         if (ke->key()==Key_Up)
         {
            prevCmd();
            return true;
         }
         else if (ke->key()==Key_Down)
         {
            nextCmd();
            return true;
         }
      }
      else if (ke->state()==ControlButton)
      {
         if (ke->key()==Key_C)
         {
//            cerr<<"c";
            m_keyCode=3;
            sendByte(m_keyCode, 0);
//            if (m_firstRep)
               m_keyRepeatTimer.start(0, false);
            return true;
         }
         else if (ke->key()==Key_Q)
         {
//            cerr<<"#";
            m_keyCode=17;
            sendByte(m_keyCode, 0);
//            m_keyRepeatTimer.start(200, true);
            return true;
         }
         else if (ke->key()==Key_S)
         {
            m_keyCode=19;
            sendByte(m_keyCode, 0);
//            m_keyRepeatTimer.start(200, true);
            return true;
         }
      }
   }
//   else if ((watched==m_cmdLe) && (e->type()==QEvent::KeyRelease) && (ke->key()==Key_Control))
   else if ((watched==m_cmdLe) && (e->type()==QEvent::KeyRelease))
   {
      m_keyRepeatTimer.stop();
//      m_firstRep=true;
   }

   return false;
}

void QCPPDialogImpl::sendKey()
{
//   cerr<<"-";
   sendByte(m_keyCode, 0);
/*   if (m_firstRep)
   {
      m_firstRep=false;
      m_keyRepeatTimer.start(1, false);
   }*/
}

void QCPPDialogImpl::oldCmdClicked(QListBoxItem* item)
{
   if (item==0)
      return;
   int index=m_oldCmdsLb->index(item);
   m_cmdLe->setText(item->text());
   m_cmdBufIndex=m_oldCmdsLb->count()-index;
   m_cmdLe->setFocus();
}

void QCPPDialogImpl::prevCmd()
{
   if (m_oldCmdsLb->count()<=m_cmdBufIndex)
      return;
//   cerr<<"prevCmd() count: "<<m_oldCmdsLb->count()<<" bufIndex: "<<
   m_cmdBufIndex++;
   m_oldCmdsLb->setCurrentItem(m_oldCmdsLb->count()-m_cmdBufIndex);
   m_oldCmdsLb->setSelected(m_oldCmdsLb->currentItem(), true);
   m_cmdLe->setText(m_oldCmdsLb->currentText());

//   cerr<<"prev() count: "<<m_oldCmdsLb->count()<<" bufIndex: "<<m_cmdBufIndex<<endl;
}

void QCPPDialogImpl::nextCmd()
{
   if (m_cmdBufIndex==0)
      return;
   m_cmdBufIndex--;
   if (m_cmdBufIndex==0)
   {
      m_cmdLe->clear();
      m_oldCmdsLb->setSelected(m_oldCmdsLb->currentItem(), false);
   }
   else
   {
      m_oldCmdsLb->setCurrentItem(m_oldCmdsLb->count()-m_cmdBufIndex);
      m_cmdLe->setText(m_oldCmdsLb->currentText());
   }
//   cerr<<"next() count: "<<m_oldCmdsLb->count()<<" bufIndex: "<<m_cmdBufIndex<<endl;
}

void QCPPDialogImpl::execCmd()
{
   m_cmdBufIndex=0;
   QString cmd=m_cmdLe->text().stripWhiteSpace();
   m_cmdLe->clear();
   if (!cmd.isEmpty())
   {
      if ((m_oldCmdsLb->count()<1) || (m_oldCmdsLb->text(m_oldCmdsLb->count()-1)!=cmd))
      {
         m_oldCmdsLb->insertItem(cmd);
         m_oldCmdsLb->setCurrentItem(m_oldCmdsLb->count()-1);
//       m_oldCmdsLb->setSelected(m_oldCmdsLb->currentItem(), false);
         if (m_oldCmdsLb->count()>50)
            m_oldCmdsLb->removeItem(0);
         saveSettings();
      }
   }
   m_oldCmdsLb->clearSelection();
   if (m_fd==-1)
      return;

   sendString(cmd);

/*   cerr<<"paras: "<<m_outputView->paragraphs()<<endl;
   if (m_outputView->paragraphs()>1100)
   {
      m_outputView->setUpdatesEnabled(false);
      m_outputView->setSelection(0, 0, 100, 0);
      m_outputView->removeSelectedText();
      m_outputView->scrollToBottom();
      m_outputView->append("abc\n");
      m_outputView->setUpdatesEnabled(true);
   }
   else*/
/*   m_outputView->setCursorPosition(m_outputView->paragraphs()-1, 0);
   static int i=0;
   i++;
   QString s=QString::number(i);
   s=" -"+s+"-\n ";
   m_outputView->insert(s);*/
/*   m_outputView->insert("abc");
   m_outputView->insert("def");
   m_outputView->insert("ghi\n");
   m_outputView->update();*/
}

bool QCPPDialogImpl::sendString(const QString& s)
{
   unsigned int lineMode=m_inputModeCb->currentItem();
   unsigned int charDelay=m_charDelaySb->value();

   if (lineMode==4) // hex
   {
      QString hex=s;
      hex.remove(QRegExp("\\s"));
      if ((hex.startsWith("0x")) || (hex.startsWith("0X")))
         hex=hex.mid(2);

      if (hex.length()%2 != 0)
         hex="0"+hex;

      for (unsigned int i=0; i<hex.length()/2; i++)
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

      for (unsigned int i=0; i<hex.length()/2; i++)
      {
         QString nextByte=hex.mid(i*2, 2);
         unsigned int byte=nextByte.toUInt(0, 16);
         sendByte(byte & 0xff, charDelay);
         fprintf(stderr, " 0x%x d:%d ", byte & 0xff, charDelay);
      }
      return true;
   }

   const char *bytes=s.latin1();
   for (unsigned int i=0; i<s.length(); i++)
   {
      if (!sendByte(*bytes, charDelay))
         return false;
      bytes++;
   }

   if (lineMode==0)
   {
      if (!sendByte('\n', charDelay))
         return false;
   }
   else if (lineMode==1)
   {
      if (!sendByte('\r', charDelay))
         return false;
   }
   else if (lineMode==2)
   {
      if (!sendByte('\r', charDelay))
         return false;
      if (!sendByte('\n', charDelay))
         return false;
   }
   return true;
}

bool QCPPDialogImpl::sendByte(char c, unsigned int delay)
{
   if (m_fd==-1)
      return false;
//   c=c&0xff;
   int res=::write(m_fd, &c, 1);
//   cerr<<"wrote "<<(unsigned int)(c)<<endl;
   if (res<1)
   {
      cerr<<"write returned "<<res<<" errno: "<<errno<<endl;
      perror("write\n");
      return false;
   }
   millisleep(delay);
//   else
//      cerr<<" \""<<c<<"\"; ";
   return true;
}

/*void QCPPDialogImpl::connectOrDisconnectTTY()
{
   if (m_fd==-1)
      connectTTY();
   else
      disconnectTTY();
}*/

void QCPPDialogImpl::connectTTY()
{
   QString dev=m_deviceCb->currentText();
   int baudrate=m_baudCb->currentText().toInt();
   int dataBits=m_dataBitsCb->currentText().toInt();
   QString parity=m_parityCb->currentText();
   QString stop=m_stopCb->currentText();
   bool softwareHandshake=m_softwareCb->isChecked();
   bool hardwareHandshake=m_hardwareCb->isChecked();

//  m_fd=open(dev.latin1(), O_RDWR | O_NOCTTY | O_NONBLOCK);
// m_fd=open(dev.latin1(), O_RDWR | O_NOCTTY);
   int flags=0;
   if (m_readCb->isChecked() && m_writeCb->isChecked())
      flags=O_RDWR;
   else if (!m_readCb->isChecked() && m_writeCb->isChecked())
      flags=O_WRONLY;
   else if (m_readCb->isChecked() && !m_writeCb->isChecked())
      flags=O_RDONLY;
   else
   {
      QMessageBox::information(this, tr("Error"), tr("Opening the device neither for reading nor writing doesn't seem to make much sense ;-)"));
      return;
   }

   m_fd=open(dev.latin1(), flags | O_NDELAY);
   if (m_fd<0)
   {
      cerr<<"opening failed"<<endl;
      m_fd=-1;
      QMessageBox::information(this, tr("Error"), tr("Could not open %1").arg(dev));
      return;
   }

   // flushing is to be done after opening. This prevents first read and write to be spam'ish.
   tcflush(m_fd, TCIOFLUSH);

   if (m_applyCb->isChecked())
   {
      int n = fcntl(m_fd, F_GETFL, 0);
      fcntl(m_fd, F_SETFL, n & ~O_NDELAY);

      if (tcgetattr(m_fd, &m_oldtio)!=0)
         cerr<<"tcgetattr() 2 failed"<<endl;

      setNewOptions(baudrate, dataBits, parity, stop, softwareHandshake, hardwareHandshake);
   }

   m_connectPb->setEnabled(false);
   m_deviceCb->setEnabled(false);

   enableSettingWidgets(false);
   m_applyCb->setEnabled(false);

   m_readCb->setEnabled(false);
   m_writeCb->setEnabled(false);
   m_isConnected=true;


   if (m_readCb->isChecked())
   {
      m_notifier=new QSocketNotifier(m_fd, QSocketNotifier::Read, this);
      connect(m_notifier, SIGNAL(activated(int)), this, SLOT(readData(int)));
//      m_outputView->setEnabled(true);
   }
   m_oldCmdsLb->setEnabled(true);
   m_cmdLe->setEnabled(true);
   m_sendPb->setEnabled(true);
   m_protoPb->setEnabled(true);
   m_closePb->setEnabled(true);

   m_cmdLe->setFocus();
}

void QCPPDialogImpl::enableSettingWidgets(bool enable)
{
   m_baudCb->setEnabled(enable);
   m_dataBitsCb->setEnabled(enable);
   m_parityCb->setEnabled(enable);
   m_stopCb->setEnabled(enable);
   m_softwareCb->setEnabled(enable);
   m_hardwareCb->setEnabled(enable);
}

void QCPPDialogImpl::disconnectTTY()
{
   disconnectTTYRestore(m_applyCb->isChecked());
}

void QCPPDialogImpl::disconnectTTYRestore(bool restoreSettings)
{
   m_outputTimer.stop();
   m_outputBuffer="";

//   cerr<<"closing "<<m_fd<<endl;
   if (m_fd!=-1)
   {
      if (restoreSettings)
         tcsetattr(m_fd, TCSANOW, &m_oldtio);
      ::close(m_fd);
   }
   m_fd=-1;
   m_connectPb->setEnabled(true);
   m_deviceCb->setEnabled(true);

   if (m_applyCb->isChecked())
      enableSettingWidgets(true);

   m_applyCb->setEnabled(true);
   m_readCb->setEnabled(true);
   m_writeCb->setEnabled(true);

//   m_outputView->setEnabled(false);
   m_oldCmdsLb->setEnabled(false);
   m_cmdLe->setEnabled(false);
   m_sendPb->setEnabled(false);
   m_protoPb->setEnabled(false);
   m_closePb->setEnabled(false);

   m_connectPb->setFocus();

   m_isConnected=false;
   delete m_notifier;
   m_notifier=0;
}



/** This function features some code from minicom 2.0.0, src/sysdep1.c */
void QCPPDialogImpl::setNewOptions(int baudrate, int databits, const QString& parity, const QString& stop, bool softwareHandshake, bool hardwareHandshake)
{
   struct termios newtio;
//   memset(&newtio, 0, sizeof(newtio));
   if (tcgetattr(m_fd, &newtio)!=0)
   cerr<<"tcgetattr() 3 failed"<<endl;

   /*{
      unsigned int i;
      char *c =(char*)&newtio;
      fprintf(stderr,"*****************\n");
      for (i=0; i<sizeof(newtio); i++)
      {
         unsigned int t=*c;
         if (i%8 == 0)
            fprintf(stderr,"\n");
         fprintf(stderr, " 0x%02x", t&0xff);
         c++;
      }
   }*/


   speed_t _baud=0;
   switch (baudrate)
   {
   case 600:
      _baud=B600;
      break;
   case 1200:
      _baud=B1200;
      break;
   case 2400:
      _baud=B2400;
      break;
   case 4800:
      _baud=B4800;
      break;
   case 9600:
      _baud=B9600;
      break;
//   case 14400:
//      _baud=B14400;
//      break;
   case 19200:
      _baud=B19200;
      break;
//   case 28800:
//      _baud=B28800;
//      break;
   case 38400:
      _baud=B38400;
      break;
//   case 56000:
//      _baud=B56000;
//      break;
   case 57600:
      _baud=B57600;
      break;
   case 115200:
      _baud=B115200;
      break;
   case 230400:
      _baud=B230400;
      break;
   case 460800:
      _baud=B460800;
      break;
   case 576000:
      _baud=B576000;
      break;
   case 921600:
      _baud=B921600;
      break;
//   case 128000:
//      _baud=B128000;
//      break;
   default:
//   case 256000:
//      _baud=B256000;
      break;
   }
   cfsetospeed(&newtio, (speed_t)_baud);
   cfsetispeed(&newtio, (speed_t)_baud);

   /* We generate mark and space parity ourself. */
   if (databits == 7 && (parity=="Mark" || parity == "Space"))
      databits = 8;
   switch (databits)
   {
   case 5:
      newtio.c_cflag = (newtio.c_cflag & ~CSIZE) | CS5;
      break;
   case 6:
      newtio.c_cflag = (newtio.c_cflag & ~CSIZE) | CS6;
      break;
   case 7:
      newtio.c_cflag = (newtio.c_cflag & ~CSIZE) | CS7;
      break;
   case 8:
   default:
      newtio.c_cflag = (newtio.c_cflag & ~CSIZE) | CS8;
      break;
   }
   newtio.c_cflag |= CLOCAL | CREAD;

   //parity
   newtio.c_cflag &= ~(PARENB | PARODD);
   if (parity == "Even")
      newtio.c_cflag |= PARENB;
   else if (parity== "Odd")
      newtio.c_cflag |= (PARENB | PARODD);

   //hardware handshake
/*   if (hardwareHandshake)
      newtio.c_cflag |= CRTSCTS;
   else
      newtio.c_cflag &= ~CRTSCTS;*/
   newtio.c_cflag &= ~CRTSCTS;

   //stopbits
   if (stop=="2")
      newtio.c_cflag |= CSTOPB;
   else
      newtio.c_cflag &= ~CSTOPB;

//   newtio.c_iflag=IGNPAR | IGNBRK;
    newtio.c_iflag=IGNBRK;
//   newtio.c_iflag=IGNPAR;

   //software handshake
   if (softwareHandshake)
      newtio.c_iflag |= IXON | IXOFF;
   else
      newtio.c_iflag &= ~(IXON|IXOFF|IXANY);

   newtio.c_lflag=0;

   newtio.c_oflag=0;


   newtio.c_cc[VTIME]=1;
   newtio.c_cc[VMIN]=60;

//   tcflush(m_fd, TCIFLUSH);
   if (tcsetattr(m_fd, TCSANOW, &newtio)!=0)
      cerr<<"tcsetattr() 1 failed"<<endl;

   int mcs=0;
//   ioctl(m_fd, TIOCMODG, &mcs);
   ioctl(m_fd, TIOCMGET, &mcs);
   mcs |= TIOCM_RTS;
   ioctl(m_fd, TIOCMSET, &mcs);

   if (tcgetattr(m_fd, &newtio)!=0)
      cerr<<"tcgetattr() 4 failed"<<endl;
   //hardware handshake
   if (hardwareHandshake)
      newtio.c_cflag |= CRTSCTS;
   else
      newtio.c_cflag &= ~CRTSCTS;
/*  if (on)
     newtio.c_cflag |= CRTSCTS;
  else
     newtio.c_cflag &= ~CRTSCTS;*/
   if (tcsetattr(m_fd, TCSANOW, &newtio)!=0)
      cerr<<"tcsetattr() 2 failed"<<endl;

   /*{
      unsigned int i;
      char *c =(char*)&newtio;
      fprintf(stderr,"-----------------\n");
      tcgetattr(m_fd, &newtio);
      for (i=0; i<sizeof(newtio); i++)
      {
         unsigned int t=*c;
         if (i%8 == 0)
            fprintf(stderr,"\n");
         fprintf(stderr, " 0x%02x", t&0xff);
         c++;
      }
   }*/
}

void QCPPDialogImpl::readData(int fd)
{
   if (fd!=m_fd)
      return;
   int bytesRead=::read(m_fd, m_buf, CUTECOMM_BUFSIZE);

   if (bytesRead<0)
   {
      cerr<<"read result: "<<bytesRead<<endl;
      perror("read: \n");
      return;
   }

   const char* c=m_buf;
   if (m_sz!=0)
   {
//      cerr<<"readData() "<<bytesRead<<endl;
      QByteArray ba;
      ba.duplicate(m_buf, bytesRead);
//      ba.setRawData(m_buf, bytesRead);
      m_sz->writeToStdin(ba);
//      ba.resetRawData(m_buf, bytesRead);
      return;
   }

   if (m_logFile.isOpen())
   {
      m_logFile.writeBlock(m_buf, bytesRead);
   }

   QString text;
   char buf[16];
   for (int i=0; i<bytesRead; i++)
   {
      if (m_hexOutputCb->isChecked())
      {
         if ((m_hexBytes % 16) == 0)
         {
            snprintf(buf, 16, "%08x: ", m_hexBytes);
            text+=buf;
         }
         unsigned int b=*c;
         snprintf(buf, 16, "%02x ", b & 0xff);
         text+=buf;

         m_hexBytes++;
         if ((m_hexBytes % 16)==0)
            text+="\n";
         else if ((m_hexBytes % 8)==0)
            text+="  ";
      }
      else
      {
         if ((isprint(*c)) || (*c=='\n') || (*c=='\r'))
         {
            if (*c!='\r')
               text+=(*c);
         }
         else
         {
            unsigned int b=*c;
            snprintf(buf, 16, "\\0x%02x", b & 0xff);
            text+=buf;
         }
      }
      c++;
   }

   addOutput(text);
/*   if (m_outputView->paragraphs()>1100)
   {
      m_outputView->setUpdatesEnabled(false);
      m_outputView->setSelection(0, 0, 100, 0);
      m_outputView->removeSelectedText();
      m_outputView->scrollToBottom();
      m_outputView->setCursorPosition(m_outputView->paragraphs()-1, m_outputView->paragraphLength(m_outputView->paragraphs()-1));
      m_outputView->insert(text);
      m_outputView->setUpdatesEnabled(true);
   }
   else
   {
      m_outputView->setCursorPosition(m_outputView->paragraphs()-1, m_outputView->paragraphLength(m_outputView->paragraphs()-1));
      m_outputView->insert(text);
   }*/
}

void QCPPDialogImpl::addOutput(const QString& text)
{
   m_outputBuffer+=text;

   if (!m_outputTimer.isActive())
   {
      doOutput();
      m_outputTimerStart.restart();
      m_outputTimer.start(50, true);
   }
   else
   {
      if ((m_outputTimerStart.elapsed()>400)
         || ((m_outputTimerStart.elapsed()>200) && (m_outputBuffer.length()<100)))
      {
         doOutput();
         m_outputTimerStart.restart();
      }
      m_outputTimer.start(50, true);
   }
}

void QCPPDialogImpl::doOutput()
{
   if (m_outputBuffer.isEmpty())
      return;
//   cerr<<"*";
   int pNumber=m_outputView->paragraphs();
   if (pNumber>1100)
   {
      m_outputView->setUpdatesEnabled(false);
      m_outputView->setSelection(0, 0, pNumber-1000, 0);
      m_outputView->removeSelectedText();
      m_outputView->scrollToBottom();
      m_outputView->setCursorPosition(m_outputView->paragraphs()-1, m_outputView->paragraphLength(m_outputView->paragraphs()-1));
      m_outputView->insert(m_outputBuffer);
      m_outputView->setUpdatesEnabled(true);
   }
   else
   {
      m_outputView->setCursorPosition(m_outputView->paragraphs()-1, m_outputView->paragraphLength(m_outputView->paragraphs()-1));
      m_outputView->insert(m_outputBuffer);
   }
   m_outputBuffer="";
}

void QCPPDialogImpl::hexOutputClicked(bool on)
{
   addOutput("\n");
   m_hexBytes=0;
}

void QCPPDialogImpl::enableLogging(bool on)
{
   if (m_logFile.isOpen()==on)
      return;

   if (on)
   {
      m_logFile.setName(m_logFileLe->text());
      int mode=IO_ReadWrite;
      if (m_logAppendCb->currentItem()==0)
      {
         mode=mode | IO_Truncate;
      }
      else
      {
         mode=mode | IO_Append;
      }

      if (!m_logFile.open(mode))
      {
         QMessageBox::information(this, tr("Opening file failed"), tr("Could not open file %1 for writing").arg(m_logFileLe->text()));
         m_enableLoggingCb->setChecked(false);
      }
      else
      {
         m_logAppendCb->setEnabled(false);
         m_logFileLe->setEnabled(false);
         m_logFileFileDialog->setEnabled(false);
         saveSettings();
      }
   }
   else
   {
      m_logFile.close();

      m_logAppendCb->setEnabled(true);
      m_logFileLe->setEnabled(true);
      m_logFileFileDialog->setEnabled(true);

   }

}


void QCPPDialogImpl::chooseLogFile()
{
   QString logFile=QFileDialog::getSaveFileName(m_logFileLe->text());
   if (!logFile.isEmpty())
   {
      m_logFileLe->setText(logFile);
   }
}

