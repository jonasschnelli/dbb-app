// Copyright (c) 2015 Jonas Schnelli
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "dbbgui.h"

#include <QAction>
#include <QApplication>
#include <QPushButton>
#include <QDebug>
#include <QInputDialog>
#include <QMessageBox>
#include <QSpacerItem>
#include <QToolBar>

#include "ui/ui_overview.h"
#include "seeddialog.h"
#include <dbb.h>
#include "pubkey.h"
#include "base58.h"

#include "univalue/univalue.h"

#include <functional>


void executeCommand(const std::string& cmd, const std::string& password, std::function<void(const std::string&)> cmdFinished);

DBBDaemonGui::DBBDaemonGui(QWidget* parent) : QMainWindow(parent),
                                              ui(new Ui::MainWindow)
{
    QApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);
    
    ui->setupUi(this);

    qRegisterMetaType<UniValue>("UniValue");

    connect(ui->eraseButton, SIGNAL(clicked()), this, SLOT(eraseClicked()));
    connect(ui->ledButton, SIGNAL(clicked()), this, SLOT(ledClicked()));
    connect(ui->passwordButton, SIGNAL(clicked()), this, SLOT(setPasswordClicked()));
    connect(ui->seedButton, SIGNAL(clicked()), this, SLOT(seed()));
    connect(ui->joinCopayWallet, SIGNAL(clicked()), this, SLOT(JoinCopayWallet()));
    connect(ui->checkProposals, SIGNAL(clicked()), this, SLOT(checkPaymentProposals()));

    connect(this, SIGNAL(showCommandResult(const QString&)), this, SLOT(setResultText(const QString&)));
    connect(this, SIGNAL(deviceStateHasChanged(bool)), this, SLOT(changeConnectedState(bool)));

    connect(this, SIGNAL(XPubForCopayWalletIsAvailable()), this, SLOT(GetRequestXPubKey()));


    connect(this, SIGNAL(RequestXPubKeyForCopayWalletIsAvailable()), this, SLOT(JoinCopayWalletWithXPubKey()));

    connect(this, SIGNAL(gotResponse(const UniValue&, int)), this, SLOT(parseResponse(const UniValue&, int)));
    connect(this, SIGNAL(shouldVerifySigning(const QString&)), this, SLOT(showEchoVerification(const QString&)));
    connect(this, SIGNAL(signedProposalAvailable(const UniValue&, const std::vector<std::string> &)), this, SLOT(postSignedPaymentProposal(const UniValue&, const std::vector<std::string> &)));
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

    this->statusBarLabelLeft = new QLabel("");
    statusBar()->addWidget(this->statusBarLabelLeft);

    this->statusBarLabelRight = new QLabel("");
    statusBar()->addPermanentWidget(this->statusBarLabelRight);
    


    changeConnectedState(DBB::isConnectionOpen());
    setWindowTitle("The Digital Bitbox");

    bool ok;
    QString text = QInputDialog::getText(this, tr("Start Session"), tr("Current Password"), QLineEdit::Normal, "", &ok);
    if (ok && !text.isEmpty()) {
        sessionPassword = text.toStdString();
    }

    processComnand = false;

    QActionGroup *tabGroup = new QActionGroup(this);
    QAction *overviewAction = new QAction(QIcon(":/icons/home").pixmap(64), tr("&Overview"), this);
        overviewAction->setToolTip(tr("Show general overview of wallet"));
        overviewAction->setCheckable(true);
        overviewAction->setShortcut(QKeySequence(Qt::ALT + Qt::Key_1));
    tabGroup->addAction(overviewAction);
    
    QAction *copayAction = new QAction(QIcon(":/icons/copay"), tr("&Copay"), this);
        copayAction->setToolTip(tr("Show Copay wallet screen"));
        copayAction->setCheckable(true);
        copayAction->setShortcut(QKeySequence(Qt::ALT + Qt::Key_2));
    tabGroup->addAction(copayAction);
    
    QAction *settingsAction = new QAction(QIcon(":/icons/settings"), tr("&Settings"), this);
        settingsAction->setToolTip(tr("Show Settings wallet screen"));
        settingsAction->setCheckable(true);
        settingsAction->setShortcut(QKeySequence(Qt::ALT + Qt::Key_3));
    tabGroup->addAction(settingsAction);
     
    QToolBar *toolbar = addToolBar(tr("Tabs toolbar"));
            toolbar->setMovable(false);
            toolbar->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
            toolbar->addAction(overviewAction);
            toolbar->addAction(copayAction);
            toolbar->addAction(settingsAction);
            overviewAction->setChecked(true);
    toolbar->setStyleSheet("QToolButton{padding: 5px; font-size:11pt;}");
    
    connect(overviewAction, SIGNAL(triggered()), this, SLOT(gotoOverviewPage()));
    connect(copayAction, SIGNAL(triggered()), this, SLOT(gotoMultisigPage()));
    connect(settingsAction, SIGNAL(triggered()), this, SLOT(gotoSettingsPage()));
    
    //load local pubkeys
    DBBMultisigWallet copayWallet;
    copayWallet.client.LoadLocalData();
    vMultisigWallets.push_back(copayWallet);

    versionStringLoaded = false;
    versionString = "";
    
    this->ui->versionLabel->setText("loading info...");
    this->ui->nameLabel->setText("loading info...");
    getInfo(0);
}

void DBBDaemonGui::gotoOverviewPage()
{
    this->ui->stackedWidget->setCurrentIndex(0);
}

void DBBDaemonGui::gotoMultisigPage()
{
    this->ui->stackedWidget->setCurrentIndex(1);
}

void DBBDaemonGui::gotoSettingsPage()
{
    this->ui->stackedWidget->setCurrentIndex(2);
}


void DBBDaemonGui::showEchoVerification(QString echoStr)
{
    QMessageBox::information(this, tr("Verify"),
                             tr("ToDo Verify (%1)").arg(echoStr),
                             QMessageBox::Ok);
}

void DBBDaemonGui::postSignedPaymentProposal(const UniValue& proposal, const std::vector<std::string> &vSigs)
{
    vMultisigWallets[0].client.PostSignaturesForTxProposal(proposal, vSigs);
}



bool DBBDaemonGui::checkPaymentProposals()
{
    bool ret = false;
    int copayerIndex = INT_MAX;

    std::string walletsResponse;
    bool walletsAvailable = vMultisigWallets[0].client.GetWallets(walletsResponse);
    UniValue response;
    if (response.read(walletsResponse)) {
        if (response.isObject()) {
            printf("Wallet: %s\n", response.write(true, 2).c_str());

            std::string currentXPub = vMultisigWallets[0].client.GetXPubKey();
            UniValue wallet = find_value(response, "wallet");
            UniValue copayers = find_value(wallet, "copayers");
            for (const UniValue& copayer : copayers.getValues()) {
                UniValue copayerXPub = find_value(copayer, "xPubKey");
                if (!copayerXPub.isNull()) {
                    if (currentXPub == copayerXPub.get_str()) {
                        UniValue addressManager = find_value(copayer, "addressManager");
                        UniValue copayerIndexObject = find_value(addressManager, "copayerIndex");
                        copayerIndex = copayerIndexObject.get_int();
                    }
                }
            }

            UniValue pendingTxps;
            pendingTxps = find_value(response, "pendingTxps");
            if (!pendingTxps.isNull() && pendingTxps.isArray()) {
                printf("pending txps: %s", pendingTxps.write(2, 2).c_str());
                std::vector<UniValue> values = pendingTxps.getValues();
                if (values.size() == 0)
                    return false;

                bool ok;

                QString amount;
                QString toAddress;

                UniValue toAddressUni = find_value(values[0], "toAddress");
                UniValue amountUni = find_value(values[0], "amount");
                if (toAddressUni.isStr())
                    toAddress = QString::fromStdString(toAddressUni.get_str());
                if (amountUni.isNum())
                    amount = QString::number(((double)amountUni.get_int64()/100000000.0));

                QMessageBox::StandardButton reply = QMessageBox::question(this, tr("Payment Proposal Available"), tr("Do you want to sign: pay %1BTC to %2").arg(amount, toAddress), QMessageBox::Yes|QMessageBox::No);
                if (reply == QMessageBox::No)
                    return false;

                std::vector<std::pair<std::string, uint256> > inputHashesAndPaths;
                vMultisigWallets[0].client.ParseTxProposal(values[0], inputHashesAndPaths);

                std::string command = "{\"sign\": { \"type\": \"hash\", \"data\" : \"" + BitPayWalletClient::ReversePairs(inputHashesAndPaths[0].second.GetHex()) + "\", \"keypath\" : \"" + vMultisigWallets[0].baseKeyPath + "/45'/" + inputHashesAndPaths[0].first + "\" }}";
                //printf("Command: %s\n", command.c_str());

                //command = "{\"sign\": { \"type\": \"meta\", \"meta\" : \"somedata\", \"data\" : [ { \"hash:\" : \"" + BitPayWalletClient::ReversePairs(inputHashesAndPaths[0].second.GetHex()) + "\", \"keypath\" : \"" + vMultisigWallets[0].baseKeyPath + "/45'/" + inputHashesAndPaths[0].first + "\" } ], \"checkpub\" : [] } }";
                printf("Command: %s\n", command.c_str());

                executeCommand(command, sessionPassword, [&ret, values, inputHashesAndPaths, this](const std::string& cmdOut) {
                        //send a signal to the main thread
                    printf("cmd back: %s\n", cmdOut.c_str());
                    UniValue jsonOut(UniValue::VOBJ);
                    jsonOut.read(cmdOut);
                    
                    UniValue echoStr = find_value(jsonOut, "echo");
                    if (!echoStr.isNull() && echoStr.isStr())
                    {

                        emit shouldVerifySigning(QString::fromStdString(echoStr.get_str()));
                    }
                    else
                    {                    
                        UniValue signObject = find_value(jsonOut, "sign");
                        if (signObject.isArray()) {
                            std::vector<UniValue> vSignatureObjects;
                            vSignatureObjects = signObject.getValues();
                            if (vSignatureObjects.size() > 0) {
                                UniValue sigObjetc = find_value(vSignatureObjects[0], "sig");
                                UniValue pubKey = find_value(signObject, "pubkey");
                                if (!sigObjetc.isNull() && sigObjetc.isStr())
                                {                    
                                    //TODO: verify signature
                            
                                    std::vector<std::string> sigs;
                                    sigs.push_back(sigObjetc.get_str());
                                    emit signedProposalAvailable(values[0], sigs);
                                    ret = true;
                                    //client.BroadcastProposal(values[0]);
                                }
                            }
                        }
                        
                    }
                });
            }
        }
    }
    return ret;
}
bool DBBDaemonGui::sendCommand(const std::string& cmd, const std::string& password, int tag)
{
    //ensure we don't fill the queue
    //at the moment the UI should only post one command into the queue

    if (processComnand) {
        qDebug() << "Already processing a command\n";
        return false;
    }
    this->statusBarLabelRight->setText("processing...");
    this->ui->textEdit->setText("processing...");
    processComnand = true;
    executeCommand(cmd, password, [this, tag](const std::string& cmdOut) {
            //send a signal to the main thread
        UniValue jsonOut;
        jsonOut.read(cmdOut);
        emit gotResponse(jsonOut, tag);
    });
    return true;
}

void DBBDaemonGui::setResultText(const QString& result)
{
    processComnand = false;
    qDebug() << "SetResultText Called\n";
    this->ui->textEdit->setText(result);
    this->statusBarLabelRight->setText("");
}

DBBDaemonGui::~DBBDaemonGui()
{
}

void DBBDaemonGui::changeConnectedState(bool state)
{
    if (state) {
        this->statusBarLabelLeft->setText("Device Connected");
        this->statusBarButton->setVisible(true);
    } else {
        this->statusBarLabelLeft->setText("No Device Found");
        this->statusBarButton->setVisible(false);
    }

    //this->ui->widget->setEnabled(state);
}

void DBBDaemonGui::eraseClicked()
{
    std::string password;
    sendCommand("{\"reset\" : \"__ERASE__\"}", password); //no password required
    vMultisigWallets[0].client.RemoveLocalData();
    sessionPassword.clear();
}

void DBBDaemonGui::ledClicked()
{
    sendCommand("{\"led\" : \"toggle\"}", sessionPassword);
}



void DBBDaemonGui::parseResponse(const UniValue &response, int tag)
{
    processComnand = false;
    
    if (response.isObject())
    {
        if (tag == 0)
        {
            UniValue versionObj = find_value(response, "version");
            if ( versionObj.isStr())
                this->ui->versionLabel->setText(QString::fromStdString(versionObj.get_str()));
        }
        else if (tag == 1)
        {
            UniValue nameObj = find_value(response, "name");
            if ( nameObj.isStr())
                this->ui->nameLabel->setText(QString::fromStdString(nameObj.get_str()));
        }
        else if (tag == 120)
        {
            UniValue touchbuttonObj = find_value(response, "touchbutton");
            UniValue seedObj = find_value(response, "seed");
            QString errorString;
            if (!touchbuttonObj.isNull() && touchbuttonObj.isObject())
            {
                UniValue errorObj = find_value(touchbuttonObj, "error");
                if (!errorObj.isNull() && errorObj.isStr())
                    errorString = QString::fromStdString(errorObj.get_str());
            }
            if (!seedObj.isNull() && seedObj.isStr() && seedObj.get_str() == "success")
            {
                QMessageBox::information(this, tr("Wallet Created"), tr("Your wallet has been created successfully!"), QMessageBox::Ok);
            }
            else
            {
                QMessageBox::warning(this, tr("Wallet Error"), tr("Could not initialize your wallet (error: %1)!").arg(errorString), QMessageBox::Ok);
            }
        }
        else if (tag == 130)
        {
            UniValue passwordObj = find_value(response, "password");
            UniValue touchbuttonObj = find_value(response, "touchbutton");
            QString errorString;
            if (!touchbuttonObj.isNull() && touchbuttonObj.isObject())
            {
                UniValue errorObj = find_value(touchbuttonObj, "error");
                if (!errorObj.isNull() && errorObj.isStr())
                    errorString = QString::fromStdString(errorObj.get_str());
            }

            if (!passwordObj.isNull() && passwordObj.isStr() && passwordObj.get_str() == "success")
            {
                QMessageBox::information(this, tr("Password Set"), tr("Password has been set successfully!"), QMessageBox::Ok);
            }
            else
            {
                //reset password in case of an error
                sessionPassword = sessionPasswordDuringChangeProcess;
                sessionPasswordDuringChangeProcess.clear();
                
                QMessageBox::warning(this, tr("Password Error"), tr("Could not set password (error: %1)!").arg(errorString), QMessageBox::Ok);
            }
        }
        else if(tag == 180)
        {
            UniValue xPubKeyUV = find_value(response, "xpub");
            QString errorString;

            if (!xPubKeyUV.isNull() && xPubKeyUV.isStr())
            {

                SelectParams(CBaseChainParams::MAIN);
                CBitcoinExtPubKey b58PubkeyDecodeCheck(xPubKeyUV.get_str());
                CExtPubKey pubKey = b58PubkeyDecodeCheck.GetKey();

                SelectParams(CBaseChainParams::TESTNET);
                CBitcoinExtPubKey newKey(pubKey);
                std::string  xPubKeyNew = newKey.ToString();


                vMultisigWallets[0].client.setMasterPubKey(xPubKeyNew);
                emit XPubForCopayWalletIsAvailable();
            }
            else
            {
                if (xPubKeyUV.isObject())
                {
                    UniValue errorObj = find_value(xPubKeyUV, "error");
                    if (!errorObj.isNull() && errorObj.isStr())
                        errorString = QString::fromStdString(errorObj.get_str());
                }

                QMessageBox::warning(this, tr("Join Wallet Error"), tr("Error joining Copay Wallet (%1)").arg(errorString), QMessageBox::Cancel);
            }
        }
        else if(tag == 185)
        {
            UniValue requestXPubKeyUV = find_value(response, "xpub");
            QString errorString;
            
            if (!requestXPubKeyUV.isNull() && requestXPubKeyUV.isStr())
            {
                SelectParams(CBaseChainParams::MAIN);
                CBitcoinExtPubKey b58PubkeyDecodeCheck(requestXPubKeyUV.get_str());
                CExtPubKey pubKey = b58PubkeyDecodeCheck.GetKey();

                SelectParams(CBaseChainParams::TESTNET);
                CBitcoinExtPubKey newKey(pubKey);
                std::string  xRequestKeyNew = newKey.ToString();

                vMultisigWallets[0].client.setRequestPubKey(xRequestKeyNew);

                emit RequestXPubKeyForCopayWalletIsAvailable();
            }
            else
            {
                if (requestXPubKeyUV.isObject())
                {
                    UniValue errorObj = find_value(requestXPubKeyUV, "error");
                    if (!errorObj.isNull() && errorObj.isStr())
                        errorString = QString::fromStdString(errorObj.get_str());
                }

                QMessageBox::warning(this, tr("Join Wallet Error"), tr("Error joining Copay Wallet (%1)").arg(errorString), QMessageBox::Cancel);
            }
        }
        else
        {
            //general non specific response
            qDebug() << "SetResultText Called\n";
            this->ui->textEdit->setText(QString::fromStdString(response.write()));
            this->statusBarLabelRight->setText("");
        }
    }

    if (tag == 0)
    {
        emit getInfo(1);
    }
    else if (tag == 1)
    {
        qDebug() << versionString;
        versionStringLoaded = true;
    }

}

void DBBDaemonGui::getInfo(int step)
{
    versionStringLoaded = false;
    std::string command = "{\"device\":\"version\"}";
    if (step == 1)
        command = "{\"name\":\"\"}";
    
    int stepTrans = step;
    executeCommand(command, sessionPassword, [this, stepTrans](const std::string& cmdOut) {
        UniValue jsonOut;
        jsonOut.read(cmdOut);
        emit gotResponse(jsonOut, stepTrans);
    });
}

void DBBDaemonGui::setPasswordClicked()
{
    bool ok;
    QString text = QInputDialog::getText(this, tr("Set New Password"), tr("Password"), QLineEdit::Normal, "0000", &ok);
    if (ok && !text.isEmpty()) {
        std::string command = "{\"password\" : \"" + text.toStdString() + "\"}";

        executeCommand(command, sessionPassword, [this](const std::string& cmdOut) {
            UniValue jsonOut;
            jsonOut.read(cmdOut);
            emit gotResponse(jsonOut, 130);
        });

        // change the password, assuming the new password could be set
        // possible chance of error
        // TODO: need to store old password in case of an unsuccessfull password change
        sessionPasswordDuringChangeProcess = sessionPassword;
        sessionPassword = text.toStdString();
    }

}

void DBBDaemonGui::seed()
{
    std::string command = "{\"seed\" : {\"source\" :\"create\","
                        "\"decrypt\": \"no\","
                        "\"salt\" : \"\"} }";
    
    executeCommand(command, sessionPassword, [this](const std::string& cmdOut) {
        UniValue jsonOut;
        jsonOut.read(cmdOut);
        emit gotResponse(jsonOut, 120);
    });
}

void DBBDaemonGui::JoinCopayWallet()
{
    setResultText(QString::fromStdString(""));

    if (!vMultisigWallets[0].client.IsSeeded()) {
        //if there is no xpub and request key, seed over DBB
        setResultText("Initializing Copay Client... this might take some seconds.");
        GetXPubKey();
    } else {
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
    bool ret = vMultisigWallets[0].client.JoinWallet("digitalbitbox", text.toStdString(), result);

    if (!ret) {
        UniValue responseJSON;
        std::string additionalErrorText = "unknown";
        if (responseJSON.read(result)) {
            UniValue errorText;
            errorText = find_value(responseJSON, "message");
            if (!errorText.isNull() && errorText.isStr())
                additionalErrorText = errorText.get_str();
        }
        setResultText(QString::fromStdString(result));

        int ret = QMessageBox::warning(this, tr("Copay Wallet Response"), tr("Joining the wallet failed (%1)").arg(QString::fromStdString(additionalErrorText)), QMessageBox::Ok);
    } else {
        QMessageBox::information(this, tr("Copay Wallet Response"), tr("Successfull joined Copay Wallet"), QMessageBox::Ok);
    }
}

void DBBDaemonGui::GetXPubKey()
{
    sendCommand("{\"xpub\":\"" + vMultisigWallets[0].baseKeyPath + "/45'\"}", sessionPassword, 180);
}

void DBBDaemonGui::GetRequestXPubKey()
{
    //try to get the xpub for seeding the request private key (ugly workaround)
    //we cannot export private keys from a hardware wallet
    sendCommand("{\"xpub\":\"" + vMultisigWallets[0].baseKeyPath + "/1'/0\"}", sessionPassword, 185);
}


void DBBDaemonGui::JoinCopayWalletWithXPubKey()
{
    //set the keys and try to join the wallet
    _JoinCopayWallet();
}