#include "include/app.h"

#include "include/chartdialog.h"
#include "include/qtcoreclient.h"

#include <QThread>

App::App(int &argc, char **argv)
    : QApplication(argc, argv)
    , m_login_dialog(new LoginDialog())
    , m_main_window(new MainWindow())
{
    m_login_dialog->setHidden(true);
    m_main_window->setHidden(true);

    connect(m_login_dialog, &LoginDialog::brokerageCenterConnected, this, &App::startConnection);

    this->loadTheme();

    //! Trigger login window after QEventLoop of App is initialized
    QTimer::singleShot(1, [this](){login();});
}

App::~App()
{
    qDebug()<<"~App()";
    stopConnection();
    if(m_main_window)
        m_main_window->deleteLater();
    if(m_login_dialog)
        m_login_dialog->deleteLater();
}

void App::login()
{
    if (!m_login_dialog->exec() == QDialog::Accepted) {
        this->quit();
    }
}

void App::loadTheme()
{
    QFile styleFile(":/theme/dark.theme");
    styleFile.open(QFile::ReadOnly);
    this->setStyleSheet(styleFile.readAll());
    styleFile.close();
}

void App::startConnection()
{
    m_login_dialog->close();
    m_main_window->setHidden(false);
    QThread::sleep(3);
    QtCoreClient::getInstance().adaptStocklist();
}

void App::stopConnection()
{
    shift::FIXInitiator::getInstance().disconnectBrokerageCenter();
    QtCoreClient::getInstance().deleteLater();
}
