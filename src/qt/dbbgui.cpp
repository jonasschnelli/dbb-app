// Copyright (c) 2015 Jonas Schnelli
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "dbbgui.h"

#include <QApplication>
#include <QPushButton>
#include <QDebug>
#include <QInputDialog>
#include <QSpacerItem>

#include "ui/ui_overview.h"
#include "seeddialog.h"
#include <dbb.h>
#include "libbitpay-wallet-client/bpwalletclient.h"

#include <functional>


void executeCommand(const std::string& cmd, const std::string& password, std::function<void(const std::string&)> cmdFinished);

DBBDaemonGui::DBBDaemonGui(QWidget* parent) : QMainWindow(parent),
                                              ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    connect(ui->eraseButton, SIGNAL(clicked()), this, SLOT(eraseClicked()));
    connect(ui->ledButton, SIGNAL(clicked()), this, SLOT(ledClicked()));
    connect(ui->passwordButton, SIGNAL(clicked()), this, SLOT(setPasswordClicked()));
    connect(ui->seedButton, SIGNAL(clicked()), this, SLOT(seed()));
    connect(ui->joinCopayWallet, SIGNAL(clicked()), this, SLOT(JoinCopayWallet()));

    connect(this, SIGNAL(showCommandResult(const QString&)), this, SLOT(setResultText(const QString&)));
    connect(this, SIGNAL(deviceStateHasChanged(bool)), this, SLOT(changeConnectedState(bool)));

    //set window icon
    QApplication::setWindowIcon(QIcon(":/icons/dbb"));

    //set status bar
    QWidget* spacer = new QWidget();
    spacer->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    spacer->setMinimumWidth(3);
    spacer->setMaximumHeight(10);
    statusBar()->addWidget(spacer);
    this->statusBarButton = new QPushButton(QIcon(":/icons/connected"), "");
    this->statusBarButton->setEnabled(false);
    this->statusBarButton->setFlat(true);
    this->statusBarButton->setMaximumWidth(18);
    this->statusBarButton->setMaximumHeight(18);
    this->statusBarButton->setVisible(false);
    statusBar()->addWidget(this->statusBarButton);

    this->statusBarLabel = new QLabel("");
    statusBar()->addWidget(this->statusBarLabel);


    changeConnectedState(DBB::isConnectionOpen());
    setWindowTitle("The Digital Bitbox");

    bool ok;
    QString text = QInputDialog::getText(this, tr("Start Session"), tr("Current Password"), QLineEdit::Normal, "", &ok);
    if (ok && !text.isEmpty()) {
        sessionPassword = text.toStdString();
    }

    processComnand = false;
}

bool DBBDaemonGui::sendCommand(const std::string& cmd, const std::string& password)
{
    //ensure we don't fill the queue
    //at the moment the UI should only post one command into the queue

    if (processComnand) {
        qDebug() << "Already processing a command\n";
        return false;
    }
    this->ui->textEdit->setText("processing...");
    processComnand = true;
    executeCommand(cmd, password, [this](const std::string& cmdOut) {
            //send a signal to the main thread
            emit showCommandResult(QString::fromStdString(cmdOut));
    });
    return true;
}

void DBBDaemonGui::setResultText(const QString& result)
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
    if (state) {
        this->statusBarLabel->setText("Device connected");
        this->statusBarButton->setVisible(true);
    } else {
        this->statusBarLabel->setText("no device found");
        this->statusBarButton->setVisible(false);
    }

    //this->ui->widget->setEnabled(state);
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
    QString text = QInputDialog::getText(this, tr("Set New Password"), tr("Password"), QLineEdit::Normal, "0000", &ok);
    if (ok && !text.isEmpty()) {
        sendCommand("{\"password\" : \"" + text.toStdString() + "\"}", sessionPassword);
        sessionPassword = text.toStdString();
    }
}

void DBBDaemonGui::seed()
{
    SeedDialog* dialog = new SeedDialog();
    dialog->setWindowTitle("Dialog");
    if (dialog->exec() == 1) {
        if (dialog->SelectedWalletType() == 0) {
            sendCommand("{\"seed\" : {\"source\" :\"create\","
                        "\"decrypt\": \"no\","
                        "\"salt\" : \"\"} }",
                        sessionPassword);
        }
    }
}

void DBBDaemonGui::JoinCopayWallet()
{
    bool ok;
    QString text = QInputDialog::getText(this, tr("Join Copay Wallet"), tr("Wallet Invitation Code"), QLineEdit::Normal, "", &ok);
    if (!ok || text.isEmpty())
        return;
    
    BitPayWalletClient client;
    client.seed(); //generate a new wallet
    
    std::string result;
    bool ret = client.JoinWallet("digitalbitbox", text.toStdString(), result);
    
    setResultText(QString::fromStdString(result));
}