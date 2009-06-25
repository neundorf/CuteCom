/*  Copyright (C) 2004-2009 Alexander Neundorf <neundorf@kde.org>

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
#include <qlistwidget.h>
#include <qlineedit.h>
#include <qfiledialog.h>
#include <qdir.h>
#include <qfile.h>
#include <qmessagebox.h>
#include <qsettings.h>
#include <qevent.h>
#include <qprogressdialog.h>
#include <qapplication.h>
#include <qfileinfo.h>
#include <qregexp.h>
#include <qspinbox.h>
//Added by qt3to4:
#include <QKeyEvent>
#include <QResizeEvent>
#include <qtextbrowser.h>
#include <qtextstream.h>

#include <q3process.h>
#include <q3cstring.h>


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
:QWidget(parent)
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
,m_hexBytes(0)
,m_previousChar('\0')
{
   QCoreApplication::setOrganizationName("CuteCom");
//   QCoreApplication::setOrganizationDomain("mysoft.com");
   QCoreApplication::setApplicationName("CuteCom");

   this->setupUi(this);
   fillBaudCb();

   connect(m_connectPb, SIGNAL(clicked()), this, SLOT(connectTTY()));
   connect(m_closePb, SIGNAL(clicked()), this, SLOT(disconnectTTY()));
   connect(m_clearOutputPb, SIGNAL(clicked()), this, SLOT(clearOutput()));
//   connect(m_clearInputPb, SIGNAL(clicked()), m_oldCmdsLb, SLOT(clear()));

   connect(m_cmdLe, SIGNAL(returnPressed()), this, SLOT(execCmd()));

   connect(m_sendPb, SIGNAL(clicked()), this, SLOT(sendFile()));
   connect(m_aboutPb, SIGNAL(clicked()), this, SLOT(showAboutMsg()));
   connect(m_quitPb, SIGNAL(clicked()), this, SLOT(close()));

   connect(m_oldCmdsLb, SIGNAL(itemClicked(QListWidgetItem*)), this, SLOT(oldCmdClicked(QListWidgetItem*)));
   connect(m_oldCmdsLb, SIGNAL(itemDoubleClicked(QListWidgetItem*)), this, SLOT(execCmd()));

   connect(m_hexOutputCb, SIGNAL(toggled(bool)), this, SLOT(hexOutputClicked(bool)));

   connect(m_connectPb, SIGNAL(clicked()), this, SLOT(saveSettings()));
   connect(m_deviceCb, SIGNAL(activated(int)), this, SLOT(saveSettings()));
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

   m_outputView->setWordWrapMode(QTextOption::WrapAnywhere); 
   m_outputView->document()->setMaximumBlockCount(500);
//  TODO ? m_outputView->setWordWrap(Q3TextEdit::WidgetWidth);

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

void QCPPDialogImpl::fillBaudCb()
{
#ifdef B0
   m_baudCb->addItem("0");
#endif
   
#ifdef B50
   m_baudCb->addItem("50");
#endif
#ifdef B75
   m_baudCb->addItem("75");
#endif
#ifdef B110
   m_baudCb->addItem("110");
#endif
#ifdef B134
   m_baudCb->addItem("134");
#endif
#ifdef B150
   m_baudCb->addItem("150");
#endif
#ifdef B200
   m_baudCb->addItem("200");
#endif
#ifdef B300
   m_baudCb->addItem("300");
#endif
#ifdef B600
   m_baudCb->addItem("600");
#endif
#ifdef B1200
   m_baudCb->addItem("1200");
#endif
#ifdef B1800
   m_baudCb->addItem("1800");
#endif
#ifdef B2400
   m_baudCb->addItem("2400");
#endif
#ifdef B4800
   m_baudCb->addItem("4800");
#endif
#ifdef B7200
   m_baudCb->addItem("7200");
#endif
#ifdef B9600
   m_baudCb->addItem("9600");
#endif
#ifdef B14400
   m_baudCb->addItem("14400");
#endif
#ifdef B19200
   m_baudCb->addItem("19200");
#endif
#ifdef B28800
   m_baudCb->addItem("28800");
#endif
#ifdef B38400
   m_baudCb->addItem("38400");
#endif
#ifdef B57600
   m_baudCb->addItem("57600");
#endif
#ifdef B76800
   m_baudCb->addItem("76800");
#endif

   // this one is the default (without special reason)
   m_baudCb->addItem("115200");
   m_baudCb->setCurrentIndex(m_baudCb->count()-1);

#ifdef B128000
   m_baudCb->addItem("128000");
#endif
#ifdef B230400
   m_baudCb->addItem("230400");
#endif
#ifdef B460800
   m_baudCb->addItem("460800");
#endif
#ifdef B576000
   m_baudCb->addItem("576000");
#endif
#ifdef B921600
   m_baudCb->addItem("921600");
#endif
}

void QCPPDialogImpl::resizeEvent(QResizeEvent *e)
{
   QWidget::resizeEvent(e);
   saveSettings();
}

void QCPPDialogImpl::saveSettings()
{
   QSettings settings;
   settings.setValue("/cutecom/HardwareHandshake", m_hardwareCb->isChecked());
   settings.setValue("/cutecom/SoftwareHandshake", m_softwareCb->isChecked());

   settings.setValue("/cutecom/OpenForReading", m_readCb->isChecked());
   settings.setValue("/cutecom/OpenForWriting", m_writeCb->isChecked());
   settings.setValue("/cutecom/DontApplySettings", !m_applyCb->isChecked());

   settings.setValue("/cutecom/Device", m_deviceCb->currentText());
   settings.setValue("/cutecom/Baud", m_baudCb->currentItem());
   settings.setValue("/cutecom/Databits", m_dataBitsCb->currentItem());
   settings.setValue("/cutecom/Parity", m_parityCb->currentItem());
   settings.setValue("/cutecom/Stopbits", m_stopCb->currentItem());
   settings.setValue("/cutecom/Protocol", m_protoPb->currentItem());
   settings.setValue("/cutecom/width", width());
   settings.setValue("/cutecom/height", height());

   settings.setValue("/cutecom/LineMode", m_inputModeCb->currentItem());
   settings.setValue("/cutecom/HexOutput", m_hexOutputCb->isChecked());
   settings.setValue("/cutecom/CharDelay", m_charDelaySb->value());

   settings.setValue("/cutecom/SendFileDialogStartDir", m_sendFileDialogStartDir);
   settings.setValue("/cutecom/LogFileName", m_logFileLe->text());

   settings.setValue("/cutecom/AppendToLogFile", m_logAppendCb->currentItem());


   QString currentDevice=m_deviceCb->currentText();
   settings.setValue("/cutecom/CurrentDevice", currentDevice);
   bool currentDeviceIsInList=false;
   QStringList devices;
   for (int i=0; i<m_deviceCb->count(); i++)
   {
      QString s=m_deviceCb->itemText(i);
      devices<<s;
      if (s==currentDevice)
      {
         currentDeviceIsInList=true;
      }
   }

   if (!currentDeviceIsInList)
   {
      devices<<currentDevice;
   }

   settings.setValue("/cutecom/AllDevices", devices);

   int historyCount=m_oldCmdsLb->count();
   if (historyCount>50)
   {
      historyCount=50;
   }

   QStringList saveHist;
   for (unsigned int i=m_oldCmdsLb->count()-historyCount; i<m_oldCmdsLb->count(); i++)
   {
      saveHist << m_oldCmdsLb->item(i)->text();
   }
   settings.setValue("/cutecom/History", saveHist);
}

void QCPPDialogImpl::readSettings()
{
   QSettings settings;
   m_hardwareCb->setChecked(settings.value("/cutecom/HardwareHandshake", false).toBool());
   m_softwareCb->setChecked(settings.value("/cutecom/SoftwareHandshake", false).toBool());
   m_readCb->setChecked(settings.value("/cutecom/OpenForReading", true).toBool());
   m_writeCb->setChecked(settings.value("/cutecom/OpenForWriting", true).toBool());

   m_applyCb->setChecked(!settings.value("/cutecom/DontApplySettings", false).toBool());
   enableSettingWidgets(m_applyCb->isChecked());

   int defaultBaud = settings.value("/cutecom/Baud", -1).toInt();
   if (defaultBaud != -1)
   {
      m_baudCb->setCurrentIndex(defaultBaud);
   }
   m_dataBitsCb->setCurrentIndex(settings.value("/cutecom/Databits", 3).toInt());
   m_parityCb->setCurrentIndex(settings.value("/cutecom/Parity", 0).toInt());
   m_stopCb->setCurrentIndex(settings.value("/cutecom/Stopbits", 0).toInt());
   m_protoPb->setCurrentIndex(settings.value("/cutecom/Protocol", 0).toInt());

   m_inputModeCb->setCurrentIndex(settings.value("/cutecom/LineMode", 0).toInt());
   m_hexOutputCb->setChecked(settings.value("/cutecom/HexOutput", false).toBool());
   m_charDelaySb->setValue(settings.value("/cutecom/CharDelay", 1).toInt());

   m_sendFileDialogStartDir=settings.value("/cutecom/SendFileDialogStartDir", QDir::homePath()).toString();
   m_logFileLe->setText(settings.value("/cutecom/LogFileName", QDir::homePath()+"/cutecom.log").toString());

   m_logAppendCb->setCurrentIndex(settings.value("/cutecom/AppendToLogFile", 0).toInt());

   int x=settings.value("/cutecom/width", -1).toInt();
   int y=settings.value("/cutecom/height", -1).toInt();
   if ((x>100) && (y>100))
   {
      resize(x,y);
   }

   bool entryFound = settings.contains("/cutecom/AllDevices");
   QStringList devices=settings.value("/cutecom/AllDevices").toStringList();
   if (!entryFound)
   {
      devices<<"/dev/ttyS0"<<"/dev/ttyS1"<<"/dev/ttyS2"<<"/dev/ttyS3";
   }

   m_deviceCb->insertItems(0, devices);

   int indexOfCurrentDevice = devices.indexOf(settings.value("/cutecom/CurrentDevice", "/dev/ttyS0").toString());
   // fprintf(stderr, "currentDEev: -%s - index: %d\n", settings.value("/cutecom/CurrentDevice", "/dev/ttyS0").toString().toLatin1().constData(), indexOfCurrentDevice);
   if (indexOfCurrentDevice!=-1)
   {
       m_deviceCb->setCurrentIndex(indexOfCurrentDevice);
   }

   QStringList history=settings.value("/cutecom/History").toStringList();

   if (!history.empty())
   {
      m_oldCmdsLb->addItems(history);
      m_oldCmdsLb->setCurrentRow(m_oldCmdsLb->count()-1);
      m_oldCmdsLb->scrollToItem(m_oldCmdsLb->currentItem());
      m_oldCmdsLb->clearSelection();
   }
}

void QCPPDialogImpl::showAboutMsg()
{
   QMessageBox::about(this, tr("About CuteCom"), tr("This is CuteCom 0.22.0<br>(c)2004-2009 Alexander Neundorf, &lt;neundorf@kde.org&gt;<br>Licensed under the GNU GPL v2"));
}

void QCPPDialogImpl::sendFile()
{
   if (m_fileDlg==0)
   {
      m_fileDlg=new QFileDialog();
      m_fileDlg->setDirectory(m_sendFileDialogStartDir);
      m_fileDlg->setFileMode(QFileDialog::ExistingFile);
   }

   QString filename;
   if ( m_fileDlg->exec() == QDialog::Accepted )
   {
      QStringList fn = m_fileDlg->selectedFiles();
      if(!fn.isEmpty())
      {
          filename = fn[0];
      }
      m_sendFileDialogStartDir=m_fileDlg->directory().absolutePath();
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
      if (!file.open(QIODevice::ReadOnly))
      {
         QMessageBox::information(this, tr("Opening file failed"), tr("Could not open file %1").arg(filename));
         return;
      }

      QTextStream stream(&file);
      while (!stream.atEnd())
      {
         QString nextLine=stream.readLine();
         nextLine=nextLine.left((unsigned int)nextLine.indexOf('#'));
         if (nextLine.isEmpty())
         {
            continue;
         }

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
      if (!file.open(QIODevice::ReadOnly))
      {
         QMessageBox::information(this, tr("Opening file failed"), tr("Could not open file %1").arg(filename));
         return;
      }
      QByteArray data=file.readAll();
      delete m_progress;
      m_progress=new QProgressDialog(tr("Sending file..."), tr("Cancel"), 0, 100, this);
      m_progress->setMinimumDuration(100);
      unsigned int step=data.size()/100;
      if (step<1)
      {
         step=1;
      }
      for (int i=0; i<data.size(); i++)
      {
         if ((i%step)==0)
         {
            m_progress->setValue(i/step);
            qApp->processEvents();
         }
         sendByte(data.data()[i], charDelay);
         if ((data.data()[i]=='\n') || (data.data()[i]=='\r')) //wait twice as long after bytes which might be line ends (helps some uCs)
         {
            millisleep(charDelay);
         }
//         if (!sendByte(data.data()[i]))
         if (0)
         {
            QMessageBox::information(this, tr("Comm error"), tr("Sending failed (%1/%2").arg(i).arg(data.count()));
            break;
         }
         if ( m_progress->wasCanceled() )
         {
            break;
         }
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
      m_sz=new Q3Process(this);
      m_sz->addArgument("sh");
      m_sz->addArgument("-c");
//      QString tmp=QString("sx -vv \"")+filename+"\" < "+m_deviceCb->currentText()+" > "+m_deviceCb->currentText();
      QString tmp=QString("sz ");
      if (m_protoPb->currentText()=="XModem")
      {
         tmp+="--xmodem ";
      }
      else if (m_protoPb->currentText()=="YModem")
      {
         tmp+="--ymodem ";
      }
      else if (m_protoPb->currentText()=="ZModem")
      {
         tmp+="--zmodem ";
      }
      else if (m_protoPb->currentText()=="1kXModem")
      {
         tmp+="--xmodem --1k ";
      }

      tmp=tmp+"-vv \""+filename+"\" < "+m_deviceCb->currentText()+" > "+m_deviceCb->currentText();
      m_sz->addArgument(tmp);
      m_sz->setCommunication(Q3Process::Stderr);

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
      m_progress=new QProgressDialog(tr("Sending file via xmodem..."), tr("Cancel"), 0, 100, this);
      connect(m_progress, SIGNAL(cancelled()), this, SLOT(killSz()));
      m_progress->setMinimumDuration(100);
      QFileInfo fi(filename);
      m_progressStepSize=fi.size()/1024/100;
      if (m_progressStepSize<1)
      {
         m_progressStepSize=1;
      }
//    cerr<<"while(isRunning)"<<std::endl;
      m_progress->setValue(0);
      while (m_sz->isRunning())
      {
         qApp->processEvents();
         millisleep(10);
      }
//      cerr<<"----------------- sx done"<<std::endl;

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
   {
      return;
   }
   m_sz->tryTerminate();
}

void QCPPDialogImpl::readFromStdout()
{
   QByteArray ba=m_sz->readStdout();
//   cerr<<"readFromStdout() "<<ba.count()<<std::endl;
   unsigned int bytesToWrite=ba.count();
   char* src=ba.data();
   while (bytesToWrite>0)
   {
      int bytesWritten=::write(m_fd, src, (bytesToWrite>CUTECOMM_BUFSIZE?CUTECOMM_BUFSIZE:bytesToWrite));
      if (bytesWritten<0)
      {
//         cerr<<"readFromStdout() error "<<bytesWritten<<" , "<<bytesToWrite<<" left"<<std::endl;
         return;
      }
      src+=bytesWritten;
      bytesToWrite-=bytesWritten;

   }
}

void QCPPDialogImpl::readFromStderr()
{
   QByteArray ba=m_sz->readStderr();
//   cerr<<"readFromStderr() "<<ba.count()<<std::endl;
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

void QCPPDialogImpl::sendDone()
{
   std::cerr<<"sx exited"<<std::endl;
}

bool QCPPDialogImpl::eventFilter(QObject* watched, QEvent *e)
{
   QKeyEvent *ke=(QKeyEvent*)e;
   if ((watched==m_cmdLe)
       && (e->type()==QEvent::KeyPress))
   {
      if (ke->state()==Qt::NoModifier)
      {
         if (ke->key()==Qt::Key_Up)
         {
            prevCmd();
            return true;
         }
         else if (ke->key()==Qt::Key_Down)
         {
            nextCmd();
            return true;
         }
      }
      else if (ke->modifiers()==Qt::ControlModifier)
      {
         if (ke->key()==Qt::Key_C)
         {
//            std::cerr<<"c";
            m_keyCode=3;
            sendByte(m_keyCode, 0);
            m_keyRepeatTimer.setSingleShot(false); 
            m_keyRepeatTimer.start(0);
            return true;
         }
         else if (ke->key()==Qt::Key_Q)
         {
//            std::cerr<<"#";
            m_keyCode=17;
            sendByte(m_keyCode, 0);
            return true;
         }
         else if (ke->key()==Qt::Key_S)
         {
            m_keyCode=19;
            sendByte(m_keyCode, 0);
            return true;
         }
      }
   }
   else if ((watched==m_cmdLe) && (e->type()==QEvent::KeyRelease))
   {
      m_keyRepeatTimer.stop();
   }

   return false;
}

void QCPPDialogImpl::sendKey()
{
//   std::cerr<<"-";
   sendByte(m_keyCode, 0);
}

void QCPPDialogImpl::oldCmdClicked(QListWidgetItem* item)
{
   if (item==0)
   {
      return;
   }
   int index=m_oldCmdsLb->row(item);
   m_cmdLe->setText(item->text());
   m_cmdBufIndex=m_oldCmdsLb->count()-index;
   m_cmdLe->setFocus();
}

void QCPPDialogImpl::prevCmd()
{
   if (m_oldCmdsLb->count()<=m_cmdBufIndex)
   {
      return;
   }

//   std::cerr<<"prevCmd() count: "<<m_oldCmdsLb->count()<<" bufIndex: "<<
   m_cmdBufIndex++;
   QListWidgetItem* item=m_oldCmdsLb->item(m_oldCmdsLb->count()-m_cmdBufIndex);
   if (item!=0)
   {
      m_oldCmdsLb->setCurrentItem(item);
      m_cmdLe->setText(item->text());
   }
//   std::cerr<<"prev() count: "<<m_oldCmdsLb->count()<<" bufIndex: "<<m_cmdBufIndex<<std::endl;
}

void QCPPDialogImpl::nextCmd()
{
   if (m_cmdBufIndex==0)
   {
      return;
   }

   m_cmdBufIndex--;
   if (m_cmdBufIndex==0)
   {
      m_cmdLe->clear();
      m_oldCmdsLb->setCurrentItem(0);
//    TODO ?  m_oldCmdsLb->setCurrentIndex(0);
   }
   else
   {
      QListWidgetItem* it=m_oldCmdsLb->item(m_oldCmdsLb->count()-m_cmdBufIndex);
      m_oldCmdsLb->setCurrentItem(it);
      m_cmdLe->setText(it->text());
   }
//   std::cerr<<"next() count: "<<m_oldCmdsLb->count()<<" bufIndex: "<<m_cmdBufIndex<<std::endl;
}

void QCPPDialogImpl::execCmd()
{
   m_cmdBufIndex=0;
   QString cmd=m_cmdLe->text().trimmed();
   m_cmdLe->clear();
   if (!cmd.isEmpty())
   {
      if ((m_oldCmdsLb->count()<1) || (m_oldCmdsLb->item(m_oldCmdsLb->count()-1)->text()!=cmd))
      {
         m_oldCmdsLb->addItem(cmd);
         m_oldCmdsLb->setCurrentRow(m_oldCmdsLb->count()-1);
         if (m_oldCmdsLb->count()>50)
         {
#if QT_VERSION >= 0x040300
            m_oldCmdsLb->removeItemWidget(m_oldCmdsLb->item(0));
#else /* QT_VERSION >= 0x030000 */
            m_oldCmdsLb->setItemWidget(m_oldCmdsLb->item(0), 0);
#endif /* QT_VERSION >= 0x030000 */
         }
         saveSettings();
      }
   }
   m_oldCmdsLb->clearSelection();
   if (m_fd==-1)
   {
      return;
   }

   sendString(cmd);

/*   std::cerr<<"paras: "<<m_outputView->paragraphs()<<std::endl;
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
   unsigned int lineMode=m_inputModeCb->currentIndex();
   unsigned int charDelay=m_charDelaySb->value();

   if (lineMode==4) // hex
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

   const char *bytes=s.toLatin1();
   for (int i=0; i<s.length(); i++)
   {
      if (!sendByte(*bytes, charDelay))
      {
         return false;
      }
      bytes++;
   }

   if (lineMode==0)
   {
      if (!sendByte('\n', charDelay))
      {
         return false;
      }
   }
   else if (lineMode==1)
   {
      if (!sendByte('\r', charDelay))
      {
         return false;
      }
   }
   else if (lineMode==2)
   {
      if (!sendByte('\r', charDelay))
      {
         return false;
      }
      if (!sendByte('\n', charDelay))
      {
         return false;
      }
   }
   return true;
}

bool QCPPDialogImpl::sendByte(char c, unsigned int delay)
{
   if (m_fd==-1)
   {
      return false;
   }
   int res=::write(m_fd, &c, 1);
//   std::cerr<<"wrote "<<(unsigned int)(c)<<std::endl;
   if (res<1)
   {
      std::cerr<<"write returned "<<res<<" errno: "<<errno<<std::endl;
      perror("write\n");
      return false;
   }
   millisleep(delay);
   return true;
}


void QCPPDialogImpl::connectTTY()
{
   QString dev=m_deviceCb->currentText();
   int baudrate=m_baudCb->currentText().toInt();
   int dataBits=m_dataBitsCb->currentText().toInt();
   QString parity=m_parityCb->currentText();
   QString stop=m_stopCb->currentText();
   bool softwareHandshake=m_softwareCb->isChecked();
   bool hardwareHandshake=m_hardwareCb->isChecked();

   int flags=0;
   if (m_readCb->isChecked() && m_writeCb->isChecked())
   {
      flags=O_RDWR;
   }
   else if (!m_readCb->isChecked() && m_writeCb->isChecked())
   {
      flags=O_WRONLY;
   }
   else if (m_readCb->isChecked() && !m_writeCb->isChecked())
   {
      flags=O_RDONLY;
   }
   else
   {
      QMessageBox::information(this, tr("Error"), tr("Opening the device neither for reading nor writing doesn't seem to make much sense ;-)"));
      return;
   }

   m_fd=open(dev.toLatin1(), flags | O_NDELAY);
   if (m_fd<0)
   {
      std::cerr<<"opening failed"<<std::endl;
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
      {
         std::cerr<<"tcgetattr() 2 failed"<<std::endl;
      }

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
   }
   m_oldCmdsLb->setEnabled(true);
   m_cmdLe->setEnabled(true);
   m_sendPb->setEnabled(true);
   m_protoPb->setEnabled(true);
   m_closePb->setEnabled(true);

   m_cmdLe->setFocus();

   m_previousChar = '\0';
   m_hexBytes=0;
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

//   std::cerr<<"closing "<<m_fd<<std::endl;
   if (m_fd!=-1)
   {
      if (restoreSettings)
      {
         tcsetattr(m_fd, TCSANOW, &m_oldtio);
      }
      ::close(m_fd);
   }
   m_fd=-1;
   m_connectPb->setEnabled(true);
   m_deviceCb->setEnabled(true);

   if (m_applyCb->isChecked())
   {
      enableSettingWidgets(true);
   }

   m_applyCb->setEnabled(true);
   m_readCb->setEnabled(true);
   m_writeCb->setEnabled(true);

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
   {
      std::cerr<<"tcgetattr() 3 failed"<<std::endl;
   }

   speed_t _baud=0;
   switch (baudrate)
   {
#ifdef B0
   case      0: _baud=B0;     break;
#endif
   
#ifdef B50
   case     50: _baud=B50;    break;
#endif
#ifdef B75
   case     75: _baud=B75;    break;
#endif
#ifdef B110
   case    110: _baud=B110;   break;
#endif
#ifdef B134
   case    134: _baud=B134;   break;
#endif
#ifdef B150
   case    150: _baud=B150;   break;
#endif
#ifdef B200
   case    200: _baud=B200;   break;
#endif
#ifdef B300
   case    300: _baud=B300;   break;
#endif
#ifdef B600
   case    600: _baud=B600;   break;
#endif
#ifdef B1200
   case   1200: _baud=B1200;  break;
#endif
#ifdef B1800
   case   1800: _baud=B1800;  break;
#endif
#ifdef B2400
   case   2400: _baud=B2400;  break;
#endif
#ifdef B4800
   case   4800: _baud=B4800;  break;
#endif
#ifdef B7200
   case   7200: _baud=B7200;  break;
#endif
#ifdef B9600
   case   9600: _baud=B9600;  break;
#endif
#ifdef B14400
   case  14400: _baud=B14400; break;
#endif
#ifdef B19200
   case  19200: _baud=B19200; break;
#endif
#ifdef B28800
   case  28800: _baud=B28800; break;
#endif
#ifdef B38400
   case  38400: _baud=B38400; break;
#endif
#ifdef B57600
   case  57600: _baud=B57600; break;
#endif
#ifdef B76800
   case  76800: _baud=B76800; break;
#endif
#ifdef B115200
   case 115200: _baud=B115200; break;
#endif
#ifdef B128000
   case 128000: _baud=B128000; break;
#endif
#ifdef B230400
   case 230400: _baud=B230400; break;
#endif
#ifdef B460800
   case 460800: _baud=B460800; break;
#endif
#ifdef B576000
   case 576000: _baud=B576000; break;
#endif
#ifdef B921600
   case 921600: _baud=B921600; break;
#endif
   default:
//   case 256000:
//      _baud=B256000;
      break;
   }
   cfsetospeed(&newtio, (speed_t)_baud);
   cfsetispeed(&newtio, (speed_t)_baud);

   /* We generate mark and space parity ourself. */
   if (databits == 7 && (parity=="Mark" || parity == "Space"))
   {
      databits = 8;
   }
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
   {
      newtio.c_cflag |= PARENB;
   }
   else if (parity== "Odd")
   {
      newtio.c_cflag |= (PARENB | PARODD);
   }

   //hardware handshake
/*   if (hardwareHandshake)
      newtio.c_cflag |= CRTSCTS;
   else
      newtio.c_cflag &= ~CRTSCTS;*/
   newtio.c_cflag &= ~CRTSCTS;

   //stopbits
   if (stop=="2")
   {
      newtio.c_cflag |= CSTOPB;
   }
   else
   {
      newtio.c_cflag &= ~CSTOPB;
   }

//   newtio.c_iflag=IGNPAR | IGNBRK;
   newtio.c_iflag=IGNBRK;
//   newtio.c_iflag=IGNPAR;

   //software handshake
   if (softwareHandshake)
   {
      newtio.c_iflag |= IXON | IXOFF;
   }
   else
   {
      newtio.c_iflag &= ~(IXON|IXOFF|IXANY);
   }

   newtio.c_lflag=0;
   newtio.c_oflag=0;

   newtio.c_cc[VTIME]=1;
   newtio.c_cc[VMIN]=60;

//   tcflush(m_fd, TCIFLUSH);
   if (tcsetattr(m_fd, TCSANOW, &newtio)!=0)
   {
      std::cerr<<"tcsetattr() 1 failed"<<std::endl;
   }

   int mcs=0;
   ioctl(m_fd, TIOCMGET, &mcs);
   mcs |= TIOCM_RTS;
   ioctl(m_fd, TIOCMSET, &mcs);

   if (tcgetattr(m_fd, &newtio)!=0)
   {
      std::cerr<<"tcgetattr() 4 failed"<<std::endl;
   }

   //hardware handshake
   if (hardwareHandshake)
   {
      newtio.c_cflag |= CRTSCTS;
   }
   else
   {
      newtio.c_cflag &= ~CRTSCTS;
   }
/*  if (on)
     newtio.c_cflag |= CRTSCTS;
  else
     newtio.c_cflag &= ~CRTSCTS;*/
   if (tcsetattr(m_fd, TCSANOW, &newtio)!=0)
   {
      std::cerr<<"tcsetattr() 2 failed"<<std::endl;
   }

}

void QCPPDialogImpl::readData(int fd)
{
   if (fd!=m_fd)
   {
      return;
   }

   int bytesRead=::read(m_fd, m_buf, CUTECOMM_BUFSIZE);

   if (bytesRead<0)
   {
      std::cerr<<"read result: "<<bytesRead<<std::endl;
      perror("read: \n");
      return;
   }
   // if the device "disappeared", e.g. from USB, we get a read event for 0 bytes
   else if (bytesRead==0)
   {
      disconnectTTY();
      return;
   }

   const char* c=m_buf;
   if (m_sz!=0)
   {
//      std::cerr<<"readData() "<<bytesRead<<std::endl;
      QByteArray ba(m_buf, bytesRead);
      m_sz->writeToStdin(ba);
      return;
   }

   if (m_logFile.isOpen())
   {
      m_logFile.write(m_buf, bytesRead);
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
         {
            text+="\n";
         }
         else if ((m_hexBytes % 8)==0)
         {
            text+="  ";
         }
      }
      else
      {
         // also print a newline for \r, and print only one newline for \r\n
         if ((isprint(*c)) || (*c=='\n') || (*c=='\r'))
         {
            if (*c=='\r')
            {
               text+='\n';
            }
            else if (*c=='\n')
            {
               if (m_previousChar != '\r')
               {
                  text+='\n';
               }
            }
            else
            {
               text+=(*c);
            }

            m_previousChar = *c;
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
}

void QCPPDialogImpl::addOutput(const QString& text)
{
   m_outputBuffer+=text;

   if (!m_outputTimer.isActive())
   {
      doOutput();
      m_outputTimerStart.restart();
      m_outputTimer.setSingleShot(true); 
      m_outputTimer.start(50);
   }
   else
   {
      if ((m_outputTimerStart.elapsed()>400)
         || ((m_outputTimerStart.elapsed()>200) && (m_outputBuffer.length()<100)))
      {
         doOutput();
         m_outputTimerStart.restart();
      }
      m_outputTimer.setSingleShot(true); 
      m_outputTimer.start(50);
   }
}

void QCPPDialogImpl::doOutput()
{
   if (m_outputBuffer.isEmpty())
   {
      return;
   }

   m_outputView->append(m_outputBuffer); 
   m_outputBuffer.clear();
}

void QCPPDialogImpl::hexOutputClicked(bool /* on */)
{
   addOutput("\n");
   m_hexBytes=0;
}

void QCPPDialogImpl::clearOutput()
{
   m_outputView->clear();
   m_hexBytes=0;
}

void QCPPDialogImpl::enableLogging(bool on)
{
   if (m_logFile.isOpen()==on)
   {
      return;
   }

   if (on)
   {
      m_logFile.setFileName(m_logFileLe->text());
      QIODevice::OpenMode mode=QIODevice::ReadWrite;
      if (m_logAppendCb->currentIndex()==0)
      {
         mode=mode | QIODevice::Truncate;
      }
      else
      {
         mode=mode | QIODevice::Append;
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
   QString logFile=QFileDialog::getSaveFileName(this, tr("Save log file ..."), m_logFileLe->text());
   if (!logFile.isEmpty())
   {
      m_logFileLe->setText(logFile);
   }
}

