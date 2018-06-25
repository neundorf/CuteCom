/*
 * Copyright (c) 208 Dimitris Tassopoulos <dimtass@gmail.com>
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

#ifndef MACROSETTINGS_H
#define MACROSETTINGS_H

#include "ui_macrosettings.h"
#include <QFileDialog>
#include <QTimer>

class MacroSettings : public QDialog, private Ui::MacroSettings
{
    Q_OBJECT

public:
    enum { NUM_OF_BUTTONS = 16 };
    explicit MacroSettings(QLineEdit *inputEdit, QPushButton **mainButtons, QWidget *parent = 0);
    virtual ~MacroSettings();
    void showPanel(bool setVisible);
    QString getMacroFilename(void) { return m_macroFilename; }
    void loadFile(QString fname);

public slots:
    void macroPress();

protected slots:
    void openFile();
    void saveFile();

signals:
    void closing();
    void sendCmd();
    void fileChanged(QString);

private:
    struct macro_item {
        macro_item() = default;
        macro_item(QLineEdit *cmd, QLineEdit *name, QSpinBox *tmr_interval, QPushButton *button, QCheckBox *tmr_active,
                   QTimer *tmr)
            : cmd(cmd)
            , name(name)
            , tmr_interval(tmr_interval)
            , button(button)
            , tmr_active(tmr_active)
            , tmr(tmr)
        {
        }
        QLineEdit *cmd;
        QLineEdit *name;
        QSpinBox *tmr_interval;
        QPushButton *button;
        QCheckBox *tmr_active;
        QTimer *tmr;
    };
    int getButtonIndex(QString btnName);
    bool parseFile(QTextStream &in);
    QWidget *m_mainForm;
    QLineEdit *m_inputEdit;
    QString m_macroFilename;
    struct macro_item **m_macros;
};

#endif // MACROSETTINGS_H
