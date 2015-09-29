// Copyright (c) 2015 Jonas Schnelli
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef DBBDAEMON_QT_DAEMONGUI_H
#define DBBDAEMON_QT_DAEMONGUI_H

#include <QMainWindow>
#include <QWidget>
#include <QLabel>
#include <QPushButton>

#include "libbitpay-wallet-client/bpwalletclient.h"
#include "dbb_app.h"

namespace Ui
{
class MainWindow;
}

class DBBMultisigWallet
{
public:
    BitPayWalletClient client;
    std::string baseKeyPath;
    DBBMultisigWallet()
    {
        baseKeyPath = "m/110'";
    }
};


typedef enum DBB_RESPONSE_TYPE
{
    DBB_RESPONSE_TYPE_UNKNOWN,
    DBB_RESPONSE_TYPE_PASSWORD,
    DBB_RESPONSE_TYPE_XPUB_MS_MASTER,
    DBB_RESPONSE_TYPE_XPUB_MS_REQUEST,
    DBB_RESPONSE_TYPE_CREATE_WALLET,
    DBB_RESPONSE_TYPE_INFO,
} dbb_response_type_t;

class DBBDaemonGui : public QMainWindow
{
    Q_OBJECT

public:
    explicit DBBDaemonGui(QWidget* parent = 0);
    ~DBBDaemonGui();

    void GetXPubKey();

private:
    Ui::MainWindow* ui;
    QLabel* statusBarLabelLeft;
    QLabel* statusBarLabelRight;
    QPushButton* statusBarButton;
    bool processComnand;
    bool deviceConnected;
    std::string sessionPassword; //TODO: needs secure space / mem locking
    std::string sessionPasswordDuringChangeProcess; //TODO: needs secure space / mem locking
    QString versionString;
    bool versionStringLoaded;
    std::vector<DBBMultisigWallet> vMultisigWallets;

    bool sendCommand(const std::string& cmd, const std::string& password, dbb_response_type_t tag = DBB_RESPONSE_TYPE_UNKNOWN);
    void _JoinCopayWallet();
        
public slots:
    void askForSessionPassword();

    void eraseClicked();
    void ledClicked();
    void setResultText(const QString& result);
    void setPasswordClicked();
    void seed();
    void changeConnectedState(bool state);

    //!enable or disable loading indication in the UI
    void setLoading(bool status);
    //!check device state and do a UI update
    void checkDevice();
    //!resets device infos (in case of a disconnect)
    void resetInfos();
    //!update overview flags (wallet / lock, etc)
    void updateOverviewFlags(bool walletAvailable, bool lockAvailable, bool loading);
    void JoinCopayWallet();
    void JoinCopayWalletWithXPubKey();
    void GetRequestXPubKey();
    bool checkPaymentProposals();
    void gotoOverviewPage();
    void gotoMultisigPage();
    void gotoSettingsPage();
    void getInfo();

    void parseResponse(const UniValue& response, dbb_cmd_execution_status_t status, dbb_response_type_t tag);
    void showEchoVerification(QString echoStr);
    void postSignedPaymentProposal(const UniValue& proposal, const std::vector<std::string> &vSigs);

signals:
    void showCommandResult(const QString& result);
    void deviceStateHasChanged(bool state);
    void XPubForCopayWalletIsAvailable();
    void RequestXPubKeyForCopayWalletIsAvailable();
    void gotResponse(const UniValue& response, dbb_cmd_execution_status_t status, dbb_response_type_t tag);

    void shouldVerifySigning(const QString& signature);
    void signedProposalAvailable(const UniValue& proposal, const std::vector<std::string> &vSigs);
};

#endif
