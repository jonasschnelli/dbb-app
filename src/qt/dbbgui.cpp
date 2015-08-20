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
#include "pubkey.h"
#include "base58.h"

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
    connect(ui->checkProposals, SIGNAL(clicked()), this, SLOT(checkPaymentProposals()));

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
    
    
    //load local pubkeys
    DBBMultisigWallet copayWallet;
    copayWallet.client.LoadLocalData();
    vMultisigWallets.push_back(copayWallet);
}


bool DBBDaemonGui::checkPaymentProposals()
{
    bool ret = false;
    int copayerIndex = INT_MAX;
    
    std::string walletsResponse;
    vMultisigWallets[0].client.GetWallets(walletsResponse);
    UniValue response;
    if (response.read(walletsResponse))
    {
        if (response.isObject())
        {
            printf("Wallet: %s\n", response.write(true, 2).c_str());
            
            std::string currentXPub = vMultisigWallets[0].client.GetXPubKey();
            UniValue wallet = find_value(response, "wallet");
            UniValue copayers = find_value(wallet, "copayers");
            for(const UniValue& copayer : copayers.getValues())
            {
                UniValue copayerXPub = find_value(copayer, "xPubKey");
                if (!copayerXPub.isNull())
                {
                    if (currentXPub == copayerXPub.get_str())
                    {
                        UniValue addressManager = find_value(copayer, "addressManager");
                        UniValue copayerIndexObject = find_value(addressManager, "copayerIndex");
                        copayerIndex = copayerIndexObject.get_int();
                    }
                }
            }
            
            UniValue pendingTxps;
            pendingTxps = find_value(response, "pendingTxps");
            if (!pendingTxps.isNull() && pendingTxps.isArray() )
            {
                printf("pending txps: %s", pendingTxps.write(2, 2).c_str());
                std::vector<UniValue> values = pendingTxps.getValues();
                if (values.size() == 0)
                    return false;
                
                std::vector<std::pair<std::string,uint256> > inputHashesAndPaths;
                vMultisigWallets[0].client.ParseTxProposal(values[0], inputHashesAndPaths);
                
                std::string command = "{\"sign\": { \"type\": \"hash\", \"data\" : \""+BitPayWalletClient::ReversePairs( inputHashesAndPaths[0].second.GetHex())+"\", \"keypath\" : \""+vMultisigWallets[0].baseKeyPath+"/45'/"+inputHashesAndPaths[0].first+"\" }}";
                printf("Command: %s\n", command.c_str());

                executeCommand(command, sessionPassword, [&ret, values,inputHashesAndPaths,this](const std::string& cmdOut) {
                        //send a signal to the main thread
                    printf("cmd back: %s\n", cmdOut.c_str());
                    UniValue jsonOut(UniValue::VOBJ);
                    jsonOut.read(cmdOut);
                    
                    UniValue echoStr = find_value(jsonOut, "echo");
                    if (!echoStr.isNull() && echoStr.isStr())
                    {
                        int ret = QMessageBox::warning(this, tr("Verify"),
                                                       tr("ToDo Verify (%1)").arg(QString::fromStdString(echoStr.get_str())),
                                                       QMessageBox::Cancel);
                    }
                    else
                    {                    
                        UniValue signObject = find_value(jsonOut, "sign");
                        UniValue sigObjetc = find_value(signObject, "sig");
                        UniValue pubKey = find_value(signObject, "pubkey");
                        if (!sigObjetc.isNull() && sigObjetc.isStr())
                        {                    
                            //TODO: verify signature
                    
                            std::vector<std::string> sigs;
                            sigs.push_back(sigObjetc.get_str());
                            vMultisigWallets[0].client.PostSignaturesForTxProposal(values[0], sigs);
                            ret = true;
                            //client.BroadcastProposal(values[0]);
                        }
                        
                    }
                });
            }
        }
    }
    return ret;
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

    if (!vMultisigWallets[0].client.IsSeeded())
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
    bool ret = vMultisigWallets[0].client.JoinWallet("digitalbitbox", text.toStdString(), result);
    
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
    else
    {
        emit showCommandResult(QString::fromStdString("Sucessfully joined wallet"));
    }
}

void DBBDaemonGui::GetXPubKey()
{
    //Export external chain extended public key
    executeCommand("{\"xpub\":\""+vMultisigWallets[0].baseKeyPath+"/45'\"}", sessionPassword, [this](const std::string& cmdOut) {
            //send a signal to the main thread
        UniValue jsonOut(UniValue::VOBJ);
        jsonOut.read(cmdOut);
        UniValue xPubKeyUV = find_value(jsonOut, "xpub");
        
        printf("XPub: %s \n\n", xPubKeyUV.get_str().c_str());
        if (!xPubKeyUV.isNull() && xPubKeyUV.isStr())
        {
            
            SelectParams(CBaseChainParams::MAIN);
            CBitcoinExtPubKey b58PubkeyDecodeCheck(xPubKeyUV.get_str());
            CExtPubKey pubKey = b58PubkeyDecodeCheck.GetKey();
            
            SelectParams(CBaseChainParams::TESTNET);
            CBitcoinExtPubKey newKey(pubKey);
            std::string  xPubKeyNew = newKey.ToString();
            
            
            emit XPubForCopayWalletIsAvailable(QString::fromStdString(xPubKeyNew));
        }
        else
        {
            emit showCommandResult(QString::fromStdString("Could not load xpub (m/45'/0) key from DBB"));
        }
    });
}

void DBBDaemonGui::GetRequestXPubKey(const QString& xPub)
{
    //try to get the xpub for seeding the request private key (ugly workaround)
    //we cannot export private keys from a hardware wallet
    executeCommand("{\"xpub\":\""+vMultisigWallets[0].baseKeyPath+"/1'/0\"}", sessionPassword, [this, xPub](const std::string& cmdOut) {
        UniValue jsonOut(UniValue::VOBJ);
        jsonOut.read(cmdOut);
        UniValue requestXPubKeyUV = find_value(jsonOut, "xpub");
        if (!requestXPubKeyUV.isNull() && requestXPubKeyUV.isStr())
        {
            SelectParams(CBaseChainParams::MAIN);
            CBitcoinExtPubKey b58PubkeyDecodeCheck(requestXPubKeyUV.get_str());
            CExtPubKey pubKey = b58PubkeyDecodeCheck.GetKey();
            
            SelectParams(CBaseChainParams::TESTNET);
            CBitcoinExtPubKey newKey(pubKey);
            std::string  xRequestKeyNew = newKey.ToString();
            
            emit RequestXPubKeyForCopayWalletIsAvailable(QString::fromStdString(xRequestKeyNew), xPub);
        }
        else
        {
            emit showCommandResult(QString::fromStdString("Could not load xpub (m/1'/0') key from DBB"));
        }
    });
}


void DBBDaemonGui::JoinCopayWalletWithXPubKey(const QString& requestKey, const QString& xPubKey)
{
    //set the keys and try to join the wallet
    vMultisigWallets[0].client.setPubKeys(requestKey.toStdString(), xPubKey.toStdString());
    _JoinCopayWallet();
}