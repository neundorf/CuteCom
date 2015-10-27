/*  Copyright (C) 2004-2005 Alexander Neundorf <neundorf@kde.org>
    Copyright (C) 2015 Meinhard Ritscher <unreachable@gmx.net>

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

#include <QApplication>
#include <QCommandLineParser>

#include "qcppdialogimpl.h"

//signal handlers should be installed...
int main( int argc, char *argv[] )
{
    QApplication a( argc, argv );


    QCommandLineParser parser;
    parser.setApplicationDescription("CuteCom - Serial  Terminal");
    parser.addHelpOption();
    QCommandLineOption sessionOption(QStringList() << "s" << "session",
               QCoreApplication::translate("main", "Open a named <session>"),
               QCoreApplication::translate("main", "session"));
    parser.addOption(sessionOption);

    // Process the actual command line arguments given by the user
    parser.process(a);
    QString session = parser.value(sessionOption);

    QCPPDialogImpl w(0, session);
    w.show();
    a.connect( &a, SIGNAL( lastWindowClosed() ), &a, SLOT( quit() ) );
    return a.exec();
}

