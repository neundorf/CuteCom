/*
 * Copyright (C) 2004-2009 Alexander Neundorf <neundorf@kde.org>
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

#include "mainwindow.h"
// version.h is generated via cmake
// if you use qmake, please cp version.h.in to version.h
#include "version.h"
#include <QApplication>
#include <QIcon>
#include <QCommandLineParser>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    QCommandLineParser parser;
    parser.setApplicationDescription(
        QCoreApplication::translate("main", "CuteCom - Serial Terminal %1").arg(CuteCom_VERSION));
    parser.addHelpOption();
    QCommandLineOption sessionOption(QStringList() << "s"
                                                   << "session",
                                     QCoreApplication::translate("main", "Open a named <session>"),
                                     QCoreApplication::translate("main", "session"));
    parser.addOption(sessionOption);

    // Process the actual command line arguments given by the user
    parser.process(a);
    QString session = parser.value(sessionOption);

    MainWindow w(0, session);
    QIcon appIcon;
    appIcon.addFile(QStringLiteral(":/images/terminal.svg"));
    w.setWindowIcon(appIcon);
    w.show();

    return a.exec();
}
