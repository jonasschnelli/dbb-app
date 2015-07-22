// Copyright (c) 2015 Jonas Schnelli
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "daemongui.h"

#include <QApplication>
#include <QPushButton>
#include <QDebug>
#include <QInputDialog>

#include "ui/ui_daemon.h"
#include <dbb.h>

#include <functional>


void executeCommand(const std::string &cmd, const std::string &password, std::function<void(const std::string&)> cmdFinished);

DBBDaemonGui::DBBDaemonGui(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    connect(ui->eraseButton, SIGNAL(clicked()), this, SLOT(eraseClicked()));
    connect(ui->ledButton, SIGNAL(clicked()), this, SLOT(ledClicked()));
    connect(ui->passwordButton, SIGNAL(clicked()), this, SLOT(setPasswordClicked()));
    connect(ui->seedButton, SIGNAL(clicked()), this, SLOT(seed()));

    connect(this, SIGNAL(showCommandResult(const QString&)), this, SLOT(setResultText(const QString&)));


    changeConnectedState(DBB::openConnection());

    setWindowTitle("The Digital Bitbox");


    bool ok;
    QString text = QInputDialog::getText(this, tr("Start Session"),
                                         tr("Current Password"), QLineEdit::Normal,
                                         "", &ok);
    if (ok && !text.isEmpty())
    {
        sessionPassword = text.toStdString();
    }

    processComnand = false;

}

bool DBBDaemonGui::sendCommand(const std::string &cmd, const std::string &password)
{
    //ensure we don't fill the queue
    //at the moment the UI should only post one command into the queue

    if (processComnand)
    {
        qDebug() << "Already processing a command\n";
        return false;
    }
    this->ui->textEdit->setText("processing...");
    processComnand = true;
    executeCommand(cmd, password, [this](const std::string &cmdOut)
        {
            //send a signal to the main thread
            emit showCommandResult(QString::fromStdString(cmdOut));
        });
    return true;
}

void DBBDaemonGui::setResultText(const QString &result)
{
    processComnand = false;
    qDebug() << "SetResultText Called\n";
    this->ui->textEdit->setText(result);
}

DBBDaemonGui::~DBBDaemonGui()
{

}

void DBBDaemonGui::changeConnectedState(bool state)
{
    if (state)
        ui->connected->setVisible(true);
    else
        ui->connected->setVisible(false);
}

void DBBDaemonGui::eraseClicked()
{
    std::string password;
    sendCommand("{\"reset\" : \"__ERASE__\"}", password); //no password required
    sessionPassword.clear();
}

void DBBDaemonGui::ledClicked()
{
    sendCommand("{\"led\" : \"toggle\"}", sessionPassword);
}

void DBBDaemonGui::setPasswordClicked()
{
    bool ok;
    QString text = QInputDialog::getText(this, tr("Set New Password"),
                                         tr("Password"), QLineEdit::Normal,
                                         "0000", &ok);
    if (ok && !text.isEmpty())
    {
        sendCommand("{\"password\" : \""+text.toStdString()+"\"}", sessionPassword);
        sessionPassword = text.toStdString();
    }
}

void DBBDaemonGui::seed()
{
    sendCommand("{\"seed\" : {\"source\" :\"create\","
                        "\"decrypt\": \"no\","
                        "\"salt\" : \"\"} }", sessionPassword);
}

