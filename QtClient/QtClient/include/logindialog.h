#pragma once

#include "global.h"

#include <QDialog>

namespace Ui {
class LoginDialog;
}

/**
 * @brief Class which defined the login dialog of the application, including .cpp and .ui,
 *        extends QDialog.
 */
class LoginDialog : public QDialog {
    Q_OBJECT

public:
    explicit LoginDialog(QWidget* parent = nullptr);
    ~LoginDialog();

private:
    Ui::LoginDialog* ui;

private slots:
    void on_exit_button_clicked();
    void on_login_button_clicked();

signals:
    void brokerageCenterConnected();
};
