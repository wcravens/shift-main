#pragma once

#include "include/mainwindow.h"
#include "include/logindialog.h"

#include <QApplication>


class App : public QApplication {
    Q_OBJECT

public:
    App(int &argc, char **argv);
    ~App();

public slots:
    void startConnection();
    void stopConnection();

private:
    MainWindow* m_main_window;
    LoginDialog* m_login_dialog;
    QtCoreClient *m_qt_core_client;

    void login();
};
