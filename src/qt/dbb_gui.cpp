// Copyright (c) 2015 Jonas Schnelli
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "dbb_gui.h"

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

void executeCommand(const std::string& cmd, const std::string& password, std::function<void(const std::string&, dbb_cmd_execution_status_t status)> cmdFinished);

DBBDaemonGui::DBBDaemonGui(QWidget* parent) : QMainWindow(parent),
                                              ui(new Ui::MainWindow)
{
    QApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);
    
    ui->setupUi(this);

    qRegisterMetaType<UniValue>("UniValue");
    qRegisterMetaType<dbb_cmd_execution_status_t>("dbb_cmd_execution_status_t");
    qRegisterMetaType<dbb_response_type_t>("dbb_response_type_t");
    qRegisterMetaType<std::vector<std::string>>("std::vector<std::string>");

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
    connect(this, SIGNAL(gotResponse(const UniValue&, dbb_cmd_execution_status_t, dbb_response_type_t)), this, SLOT(parseResponse(const UniValue&, dbb_cmd_execution_status_t, dbb_response_type_t)));
    connect(this, SIGNAL(shouldVerifySigning(const QString&)), this, SLOT(showEchoVerification(const QString&)));
    connect(this, SIGNAL(signedProposalAvailable(const UniValue&, const std::vector<std::string> &)), this, SLOT(postSignedPaymentProposal(const UniValue&, const std::vector<std::string> &)));

    //set window icon
    QApplication::setWindowIcon(QIcon(":/icons/dbb"));
    setWindowTitle("The Digital Bitbox");

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

    //set status bar connection status
    changeConnectedState(DBB::isConnectionOpen());

    deviceConnected = false;
    processComnand = false;

    // tabbar
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
    
    resetInfos();
    getInfo();
}

DBBDaemonGui::~DBBDaemonGui()
{
}

void DBBDaemonGui::askForSessionPassword()
{
    //ask for session password
    bool ok;
    QString text = QInputDialog::getText(this, tr("Start Session"), tr("Current Password"), QLineEdit::Normal, "", &ok);
    if (ok && !text.isEmpty()) {
        sessionPassword = text.toStdString();
    }
}

// page switching
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

bool DBBDaemonGui::checkPaymentProposals()
{
    bool ret = false;
    int copayerIndex = INT_MAX;

    std::string walletsResponse;
    bool walletsAvailable = vMultisigWallets[0].client.GetWallets(walletsResponse);

    if (walletsAvailable)
    {
        QMessageBox::warning(this, tr("No Wallet"),
                                 tr("No Copay Wallet Available"),
                                 QMessageBox::Ok);
    }

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

                command = "{\"sign\": { \"type\": \"meta\", \"meta\" : \"somedata\", \"data\" : [ { \"hash\" : \"" + BitPayWalletClient::ReversePairs(inputHashesAndPaths[0].second.GetHex()) + "\", \"keypath\" : \"" + vMultisigWallets[0].baseKeyPath + "/45'/" + inputHashesAndPaths[0].first + "\" } ] } }";
                printf("Command: %s\n", command.c_str());

                executeCommand(command, sessionPassword, [&ret, values, inputHashesAndPaths, this](const std::string& cmdOut, dbb_cmd_execution_status_t status) {
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
                                UniValue sigObject = find_value(vSignatureObjects[0], "sig");
                                UniValue pubKey = find_value(vSignatureObjects[0], "pubkey");
                                if (!sigObject.isNull() && sigObject.isStr())
                                {                    
                                    //TODO: verify signature
                            
                                    std::vector<std::string> sigs;
                                    sigs.push_back(sigObject.get_str());
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

void DBBDaemonGui::postSignedPaymentProposal(const UniValue& proposal, const std::vector<std::string> &vSigs)
{
    vMultisigWallets[0].client.PostSignaturesForTxProposal(proposal, vSigs);
}

bool DBBDaemonGui::sendCommand(const std::string& cmd, const std::string& password, dbb_response_type_t tag)
{
    //ensure we don't fill the queue
    //at the moment the UI should only post one command into the queue

    if (processComnand) {
        qDebug() << "Already processing a command\n";
        return false;
    }
    this->ui->textEdit->setText("processing...");
    processComnand = true;
    executeCommand(cmd, password, [this, tag](const std::string& cmdOut, dbb_cmd_execution_status_t status) {
            //send a signal to the main thread
        UniValue jsonOut;
        jsonOut.read(cmdOut);
        emit gotResponse(jsonOut, status, tag);
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

void DBBDaemonGui::changeConnectedState(bool state)
{
    bool stateChanged = deviceConnected != state;
    if (state) {
        deviceConnected = true;
        this->statusBarLabelLeft->setText("Device Connected");
        this->statusBarButton->setVisible(true);
    } else {
        deviceConnected = false;
        this->statusBarLabelLeft->setText("No Device Found");
        this->statusBarButton->setVisible(false);
    }

    if (stateChanged)
    {
        checkDevice();
    }
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



void DBBDaemonGui::parseResponse(const UniValue &response, dbb_cmd_execution_status_t status, dbb_response_type_t tag)
{
    processComnand = false;
    setLoading(false);
    if (response.isObject())
    {
        if (tag == DBB_RESPONSE_TYPE_INFO)
        {
            UniValue deviceObj = find_value(response, "device");
            if (deviceObj.isObject())
            {
                UniValue version = find_value(deviceObj, "version");
                UniValue name = find_value(deviceObj, "name");
                UniValue xpub = find_value(deviceObj, "xpub");
                UniValue lock = find_value(deviceObj, "lock");
                bool walletAvailable = (xpub.isStr() && xpub.get_str().size() > 0);
                bool lockAvailable = (lock.isStr() && lock.get_str().size() > 0);

                if (version.isStr())
                    this->ui->versionLabel->setText(QString::fromStdString(version.get_str()));
                if (name.isStr())
                    this->ui->nameLabel->setText(QString::fromStdString(name.get_str()));

                updateOverviewFlags(walletAvailable,lockAvailable,false);
            }
        }
        else if (tag == DBB_RESPONSE_TYPE_CREATE_WALLET)
        {
            UniValue touchbuttonObj = find_value(response, "touchbutton");
            UniValue seedObj = find_value(response, "seed");
            UniValue errorObj = find_value(response, "error");
            QString errorString;

            if (errorObj.isObject())
            {
                UniValue errorMsgObj = find_value(errorObj, "message");
                if (errorMsgObj.isStr())
                    errorString = QString::fromStdString(errorMsgObj.get_str());
            }
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
                QMessageBox::warning(this, tr("Wallet Error"), errorString, QMessageBox::Ok);
            }
        }
        else if (tag == DBB_RESPONSE_TYPE_PASSWORD)
        {
            if (status != DBB_CMD_EXECUTION_STATUS_OK)
            {
                //could not decrypt, password was changed successfully
                QMessageBox::information(this, tr("Password Set"), tr("Password has been set successfully!"), QMessageBox::Ok);
            }
            else {
                QString errorString;
                UniValue passwordObj = find_value(response, "password");
                UniValue touchbuttonObj = find_value(response, "touchbutton");
                if (!touchbuttonObj.isNull() && touchbuttonObj.isObject())
                {
                    UniValue errorObj = find_value(touchbuttonObj, "error");
                    if (!errorObj.isNull() && errorObj.isStr())
                        errorString = QString::fromStdString(errorObj.get_str());
                }

                //reset password in case of an error
                sessionPassword = sessionPasswordDuringChangeProcess;
                sessionPasswordDuringChangeProcess.clear();

                QMessageBox::warning(this, tr("Password Error"), tr("Could not set password (error: %1)!").arg(errorString), QMessageBox::Ok);
            }
        }
        else if(tag == DBB_RESPONSE_TYPE_XPUB_MS_MASTER)
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
        else if(tag == DBB_RESPONSE_TYPE_XPUB_MS_REQUEST)
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
}

void DBBDaemonGui::checkDevice()
{
    this->ui->verticalLayoutWidget->setVisible(deviceConnected);
    this->ui->noDeviceWidget->setVisible(!deviceConnected);

    if (!deviceConnected)
    {
        resetInfos();
    }
    else
    {
        askForSessionPassword();
        getInfo();
    }
}

void DBBDaemonGui::setLoading(bool status)
{
    this->statusBarLabelRight->setText((status) ? "processing..." : "");
    //TODO, subclass label and make it animated
}

void DBBDaemonGui::resetInfos()
{
    this->ui->versionLabel->setText("loading info...");
    this->ui->nameLabel->setText("loading info...");

    updateOverviewFlags(false,false,true);
}

void DBBDaemonGui::updateOverviewFlags(bool walletAvailable, bool lockAvailable, bool loading)
{
    this->ui->walletCheckmark->setIcon(QIcon(walletAvailable ? ":/icons/okay" : ":/icons/warning"));
    this->ui->walletLabel->setText(walletAvailable ? "Wallet available" : "No wallet seeded yet");

    this->ui->lockCheckmark->setIcon(QIcon(lockAvailable ? ":/icons/okay" : ":/icons/warning"));
    this->ui->lockLabel->setText(lockAvailable ? "Device 2FA Lock" : "No 2FA set");

    if (loading)
    {
        this->ui->lockLabel->setText("loading info...");
        this->ui->walletLabel->setText("loading info...");

        this->ui->walletCheckmark->setIcon(QIcon(":/icons/warning")); //TODO change to loading...
        this->ui->lockCheckmark->setIcon(QIcon(":/icons/warning")); //TODO change to loading...
    }

}

void DBBDaemonGui::getInfo()
{
    std::string command = "{\"device\":\"info\"}";

    setLoading(true);
    executeCommand(command, sessionPassword, [this](const std::string& cmdOut, dbb_cmd_execution_status_t status) {
        UniValue jsonOut;
        jsonOut.read(cmdOut);
        emit gotResponse(jsonOut, status, DBB_RESPONSE_TYPE_INFO);
    });
}

void DBBDaemonGui::setPasswordClicked()
{
    bool ok;
    QString text = QInputDialog::getText(this, tr("Set New Password"), tr("Password"), QLineEdit::Normal, "0000", &ok);
    if (ok && !text.isEmpty()) {
        std::string command = "{\"password\" : \"" + text.toStdString() + "\"}";

        executeCommand(command, sessionPassword, [this](const std::string& cmdOut, dbb_cmd_execution_status_t status) {
            UniValue jsonOut;
            jsonOut.read(cmdOut);
            emit gotResponse(jsonOut, status, DBB_RESPONSE_TYPE_PASSWORD);
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
    
    executeCommand(command, sessionPassword, [this](const std::string& cmdOut, dbb_cmd_execution_status_t status) {
        UniValue jsonOut;
        jsonOut.read(cmdOut);
        emit gotResponse(jsonOut, status, DBB_RESPONSE_TYPE_CREATE_WALLET);
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
    sendCommand("{\"xpub\":\"" + vMultisigWallets[0].baseKeyPath + "/45'\"}", sessionPassword, DBB_RESPONSE_TYPE_XPUB_MS_MASTER);
}

void DBBDaemonGui::GetRequestXPubKey()
{
    //try to get the xpub for seeding the request private key (ugly workaround)
    //we cannot export private keys from a hardware wallet
    sendCommand("{\"xpub\":\"" + vMultisigWallets[0].baseKeyPath + "/1'/0\"}", sessionPassword, DBB_RESPONSE_TYPE_XPUB_MS_REQUEST);
}


void DBBDaemonGui::JoinCopayWalletWithXPubKey()
{
    //set the keys and try to join the wallet
    _JoinCopayWallet();
}