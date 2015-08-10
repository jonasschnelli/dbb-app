// Copyright (c) 2015 Jonas Schnelli
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "dbbgui.h"

#include <QApplication>
#include <QPushButton>
#include <QDebug>
#include <QInputDialog>
#include <QMessageBox>
#include <QSpacerItem>

#include "ui/ui_overview.h"
#include "seeddialog.h"
#include <dbb.h>

#include "univalue/univalue.h"

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

    connect(this, SIGNAL(XPubForCopayWalletIsAvailable(const QString&)), this, SLOT(GetRequestXPubKey(const QString&)));


    connect(this, SIGNAL(RequestXPubKeyForCopayWalletIsAvailable(const QString&, const QString&)), this, SLOT(JoinCopayWalletWithXPubKey(const QString&, const QString&)));

    
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
    setResultText(QString::fromStdString(""));
    
    //check if there are local stored keys
    client.LoadLocalData();
    
    if (!client.IsSeeded())
    {
        //if there is no xpub and request key, seed over DBB
        setResultText("Initializing Copay Client... this might take some seconds.");
        GetXPubKey();
    }
    else
    {
        //send a join request
        _JoinCopayWallet();
    }
}

void DBBDaemonGui::_JoinCopayWallet()
{
    bool ok;
    QString text = QInputDialog::getText(this, tr("Join Copay Wallet"), tr("Wallet Invitation Code"), QLineEdit::Normal, "", &ok);
    if (!ok || text.isEmpty())
        return;
    
    std::string result;
    bool ret = client.JoinWallet("digitalbitbox", text.toStdString(), result);
    
    if (!ret)
    {
        UniValue responseJSON;
        std::string additionalErrorText = "unknown";
        if (responseJSON.read(result))
        {
            UniValue errorText;
            errorText = find_value(responseJSON, "message");
            if (!errorText.isNull() && errorText.isStr())
                additionalErrorText = errorText.get_str();
        }
        setResultText(QString::fromStdString(result));
        
        int ret = QMessageBox::warning(this, tr("Copay Wallet Response"),
                                       tr("Joining the wallet failed (%1)").arg(QString::fromStdString(additionalErrorText)),
                                       QMessageBox::Cancel);
    }
}

void DBBDaemonGui::GetXPubKey()
{
    //Export external chain extended public key
    executeCommand("{\"xpub\":\"m/45'/0\"}", sessionPassword, [this](const std::string& cmdOut) {
            //send a signal to the main thread
        UniValue jsonOut(UniValue::VOBJ);
        jsonOut.read(cmdOut);
        UniValue xPubKeyUV = find_value(jsonOut, "xpub");
        if (!xPubKeyUV.isNull() && xPubKeyUV.isStr())
            emit XPubForCopayWalletIsAvailable(QString::fromStdString(xPubKeyUV.get_str()));
        else
            emit showCommandResult(QString::fromStdString("Could not load xpub (m/45'/0) key from DBB"));
    });
}

void DBBDaemonGui::GetRequestXPubKey(const QString& xPub)
{
    //try to get the xpub for seeding the request private key (ugly workaround)
    //we cannot export private keys from a hardware wallet
    executeCommand("{\"xpub\":\"m/1'/0\"}", sessionPassword, [this, xPub](const std::string& cmdOut) {
        UniValue jsonOut(UniValue::VOBJ);
        jsonOut.read(cmdOut);
        UniValue requestXPubKeyUV = find_value(jsonOut, "xpub");
        if (!requestXPubKeyUV.isNull() && requestXPubKeyUV.isStr())
            emit RequestXPubKeyForCopayWalletIsAvailable(xPub, QString::fromStdString(requestXPubKeyUV.get_str()));
        else
            emit showCommandResult(QString::fromStdString("Could not load xpub (m/1'/0') key from DBB"));
    });
}


void DBBDaemonGui::JoinCopayWalletWithXPubKey(const QString& requestKey, const QString& xPubKey)
{
    //set the keys and try to join the wallet
    client.setPubKeys(requestKey.toStdString(), xPubKey.toStdString());
    _JoinCopayWallet();
}