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
    QFile styleFile(":/theme/shift.theme");
    styleFile.open(QFile::ReadOnly);
    this->setStyleSheet(styleFile.readAll());
    styleFile.close();

    ui->setupUi(this);
    this->setWindowTitle("SHIFT Beta");
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
    Global::qt_core_client.setUsername(ui->UsernameText->text().toStdString());
    try {
        shift::FIXInitiator::getInstance().connectBrokerageCenter("./config/initiator.cfg"
                                                                  , &Global::qt_core_client
                                                                  , ui->PasswordText->text().toStdString());
        emit brokerageCenterConnected();
        this->accept();
    } catch (shift::ConnectionTimeout e) {
        QMessageBox msg;
        msg.setText(e.what());
        msg.exec();
    } catch (shift::IncorrectPassword e) {
        QMessageBox msg;
        msg.setText(e.what());
        msg.exec();
    }
}
