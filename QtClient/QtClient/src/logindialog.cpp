#include "include/logindialog.h"
#include "ui_logindialog.h"

#include "include/qtcoreclient.h"

#include <QMessageBox>

// LibCoreClient
#include <Exceptions.h>


LoginDialog::LoginDialog(QWidget* parent)
    : QDialog(parent)
    , ui(new Ui::LoginDialog)
{
    ui->setupUi(this);
    this->setWindowTitle("SHIFT Alpha");
}

LoginDialog::~LoginDialog()
{
    delete ui;
}

void LoginDialog::on_exit_button_clicked()
{
    this->close();
}

void LoginDialog::on_login_button_clicked()
{
    QtCoreClient::getInstance().setUsername(ui->UsernameText->text().toStdString());
    try {
        bool connected = shift::FIXInitiator::getInstance().connectBrokerageCenter("./config/initiator.cfg"
                                                                                        , &QtCoreClient::getInstance()
                                                                                        , ui->PasswordText->text().toStdString());
        if (connected) {
            emit brokerageCenterConnected();
            this->accept();
        } else {
            ui->PasswordText->clear();
            QMessageBox msg;
            msg.setText("The username or password you specified are not correct.");
            msg.exec();
        }
    } catch (shift::ConnectionTimeout e) {
        QMessageBox msg;
        msg.setText(e.what());
        msg.exec();
    }
}
