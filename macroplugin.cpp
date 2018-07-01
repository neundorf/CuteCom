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

#include "macroplugin.h"
#include "ui_macroplugin.h"

#define TRACE if (!debug) {} else qDebug()

static bool debug = false;

MacroPlugin::MacroPlugin(QFrame *parent, Settings * settings) :
    QFrame(parent),
    ui(new Ui::MacroPlugin),
    m_settings(settings)
{
    ui->setupUi(this);
    /* Plugin by default disabled, no injection, has QFrame, no injection process cmd */
    m_plugin = new Plugin(this, "Macros", this);

    /* Array of buttons that belongs to the frame that is displayed in the main
     * dialog. Because we want to handle these buttons the same way that we handle
     * the buttons from the macro dialog, then we pass that pointer to the dialog
     * and we handle all buttons there.
     */
    QPushButton **macro_buttons = new QPushButton *[MacroSettings::NUM_OF_BUTTONS] {
        ui->m_bt_macro_1,ui-> m_bt_macro_2, ui->m_bt_macro_3, ui->m_bt_macro_4, ui->m_bt_macro_5,
            ui->m_bt_macro_6, ui->m_bt_macro_7, ui->m_bt_macro_8,
            ui->m_bt_macro_9, ui->m_bt_macro_10, ui->m_bt_macro_11, ui->m_bt_macro_12,
            ui->m_bt_macro_13, ui->m_bt_macro_14, ui->m_bt_macro_15,
            ui->m_bt_macro_16,
    };
    /* Create the macro dialog */
    m_macroSettings = new MacroSettings(macro_buttons, this);
    /* Load the macro settings file that is pointed in the current session */
    m_macroSettings->loadFile(m_settings->getCurrentSession().macroFile);
    /* event to show the macro dialog */
    connect(ui->m_bt_set_macros, SIGNAL(clicked(bool)), m_macroSettings, SLOT(show()));
    /* event for when session changes and a new file needs to be loaded */
    connect(m_macroSettings, &MacroSettings::fileChanged, m_settings, [=]() {
        /* get the new macro settings file */
        m_settings->settingChanged(Settings::MacroFile, m_macroSettings->getMacroFilename());
        /* load file */
        m_macroSettings->loadFile(m_macroSettings->getMacroFilename());
//        qDebug() << "L1 session: " << m_settings->getCurrentSessionName()
//                 << ", fname: " << m_macroSettings->getMacroFilename()
//                 << ", sfname: " << m_settings->getCurrentSession().macroFile;
    });
    /* send serial string */
    connect(m_macroSettings, SIGNAL(sendCmd(QString)), this, SIGNAL(sendCmd(QString)));
    /* unload */
    connect(ui->m_bt_unload, SIGNAL(clicked(bool)), this, SLOT(removePlugin(bool)));
}

MacroPlugin::~MacroPlugin()
{
    TRACE << "[MacroPlugin] ~()";
    if (m_plugin)
        delete m_plugin;
    delete ui;
}

/**
 * @brief Return a pointer to the plugin data
 * @return
 */
const Plugin * MacroPlugin::plugin()
{
    return m_plugin;
}

/**
 * @brief [SLOT] Send unload command to the plugin manager
 */
void MacroPlugin::removePlugin(bool)
{
    emit unload(m_plugin);
}
