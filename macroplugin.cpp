#include "macroplugin.h"
#include "ui_macroplugin.h"

MacroPlugin::MacroPlugin(QWidget *parent, Ui::MainWindow * main_ui, Settings * settings) :
    QFrame(parent),
    ui(new Ui::MacroPlugin),
    m_main_ui(main_ui),
    m_settings(settings)
{
    ui->setupUi(this);
    m_pluginData = new Plugin("Macros", true, false, this, NULL);

    QPushButton **macro_buttons = new QPushButton *[MacroSettings::NUM_OF_BUTTONS] {
        ui->m_bt_macro_1,ui-> m_bt_macro_2, ui->m_bt_macro_3, ui->m_bt_macro_4, ui->m_bt_macro_5,
            ui->m_bt_macro_6, ui->m_bt_macro_7, ui->m_bt_macro_8,
            ui->m_bt_macro_9, ui->m_bt_macro_10, ui->m_bt_macro_11, ui->m_bt_macro_12,
            ui->m_bt_macro_13, ui->m_bt_macro_14, ui->m_bt_macro_15,
            ui->m_bt_macro_16,
    };
    m_macroSettings = new MacroSettings(macro_buttons, this);
    m_macroSettings->loadFile(m_settings->getCurrentSession().macroFile);
    /* event to show the macro dialog */
    connect(ui->m_bt_set_macros, SIGNAL(clicked(bool)), m_macroSettings, SLOT(show()));
    /* event for when session changes and a new file needs to be loaded */
    connect(m_macroSettings, &MacroSettings::fileChanged, m_settings, [=]() {
        m_settings->settingChanged(Settings::MacroFile, m_macroSettings->getMacroFilename());
        m_macroSettings->loadFile(m_macroSettings->getMacroFilename());
//        qDebug() << "L1 session: " << m_settings->getCurrentSessionName()
//                 << ", fname: " << m_macroSettings->getMacroFilename()
//                 << ", sfname: " << m_settings->getCurrentSession().macroFile;
    });
    connect(m_macroSettings, &MacroSettings::sendCmd, this, [=](QString cmd_str) {
        qDebug() << "MacroSettings::sendCmd";
        emit sendCmd(cmd_str);
    });
}

MacroPlugin::~MacroPlugin()
{
    qDebug() << "~MacroPlugin()";
    delete ui;
}


const Plugin * MacroPlugin::pluginData()
{
    return m_pluginData;
}
