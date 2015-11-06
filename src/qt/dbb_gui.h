// Copyright (c) 2015 Jonas Schnelli
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef DBBDAEMON_QT_DAEMONGUI_H
#define DBBDAEMON_QT_DAEMONGUI_H

#include <QMainWindow>
#include <QWidget>
#include <QLabel>
#include <QPushButton>
#include <QPropertyAnimation>

#include <functional>
#include <thread>
#include <mutex>

#include "libbitpay-wallet-client/bpwalletclient.h"
#include "dbb_app.h"
#include "backupdialog.h"
#include "paymentproposal.h"

namespace Ui
{
class MainWindow;
}

class DBBMultisigWallet
{
public:
    BitPayWalletClient client;
    std::string baseKeyPath;
    std::string participationName;
    std::string walletRemoteName;
    UniValue currentPaymentProposals;
    int64_t availableBalance;
    DBBMultisigWallet()
    {
        baseKeyPath = "m/131'";
        participationName = "digitalbitbox";
    }
};

//DBB USB response types
typedef enum DBB_RESPONSE_TYPE
{
    DBB_RESPONSE_TYPE_UNKNOWN,
    DBB_RESPONSE_TYPE_PASSWORD,
    DBB_RESPONSE_TYPE_XPUB_MS_MASTER,
    DBB_RESPONSE_TYPE_XPUB_MS_REQUEST,
    DBB_RESPONSE_TYPE_XPUB_SW_MASTER,
    DBB_RESPONSE_TYPE_XPUB_SW_REQUEST,
    DBB_RESPONSE_TYPE_CREATE_WALLET,
    DBB_RESPONSE_TYPE_INFO,
    DBB_RESPONSE_TYPE_ERASE,
    DBB_RESPONSE_TYPE_LED_BLINK,
    DBB_RESPONSE_TYPE_ADD_BACKUP,
    DBB_RESPONSE_TYPE_LIST_BACKUP,
    DBB_RESPONSE_TYPE_ERASE_BACKUP,
    DBB_RESPONSE_TYPE_RANDOM_NUM,
    DBB_RESPONSE_TYPE_DEVICE_LOCK
} dbb_response_type_t;

typedef enum DBB_PROCESS_INFOLAYER_STYLE
{
    DBB_PROCESS_INFOLAYER_STYLE_NO_INFO,
    DBB_PROCESS_INFOLAYER_STYLE_TOUCHBUTTON
} dbb_process_infolayer_style_t;

class DBBDaemonGui : public QMainWindow
{
    Q_OBJECT

public:
    explicit DBBDaemonGui(QWidget* parent = 0);
    ~DBBDaemonGui();
    void GetXPubKey(int walletIndex);

public slots:
    //!main callback when the device gets connected/disconnected
    void changeConnectedState(bool state);
    //!slot to ask for the current session password
    void askForSessionPassword();

    void eraseClicked();
    void ledClicked();
    void setResultText(const QString& result);
    void setPasswordClicked(bool showInfo=true);
    void seed();

    //backup calls
    void showBackupDialog();
    void addBackup();
    void listBackup();
    void eraseAllBackups();
    void restoreBackup(const QString& backupFilename);

    void getRandomNumber();
    //!lock the device, disabled "backup", "verifypass" and "seed" command
    void lockDevice();

    //!enable or disable dbb/usb/loading indication in the UI
    void setLoading(bool status);

    //!enable or disable net/loading indication in the UI
    void setNetLoading(bool status);

    //!resets device infos (in case of a disconnect)
    void resetInfos();
    //!update overview flags (wallet / lock, etc)
    void updateOverviewFlags(bool walletAvailable, bool lockAvailable, bool loading);
    void JoinCopayWallet(int walletIndex = -1);
    void JoinCopayWalletWithXPubKey(int walletIndex);
    void GetRequestXPubKey(int walletIndex);

    void MultisigUpdateWallets();
    void MultisigParseWalletsResponse(bool walletAvailable, const std::string &walletsResponse);
    bool MultisigUpdatePaymentProposals(const UniValue &response);
    bool MultisigDisplayPaymentProposal(const UniValue &pendingTxps, const std::string &targetID);
    void updateUIMultisigWallets(const UniValue &walletResponse);
    void PaymentProposalAction(const UniValue &paymentProposal, int actionType);
    void gotoOverviewPage();
    void gotoMultisigPage();
    void gotoSettingsPage();
    void getInfo();

    void createSingleWallet();

    void mainOverviewButtonClicked();
    void mainMultisigButtonClicked();
    void mainReceiveButtonClicked();
    void mainSendButtonClicked();
    void mainSettingsButtonClicked();


    void parseResponse(const UniValue& response, dbb_cmd_execution_status_t status, dbb_response_type_t tag, int subtag);
    void showEchoVerification(const UniValue& response, int actionType, QString echoStr);
    void postSignedPaymentProposal(const UniValue& proposal, const std::vector<std::string> &vSigs);

    void passwordProvided();
signals:
    void showCommandResult(const QString& result);
    void deviceStateHasChanged(bool state);
    void XPubForCopayWalletIsAvailable(int walletIndex);
    void RequestXPubKeyForCopayWalletIsAvailable(int walletIndex);
    void gotResponse(const UniValue& response, dbb_cmd_execution_status_t status, dbb_response_type_t tag, int subtag = 0);

    void multisigWalletResponseAvailable(bool walletsAvailable, const std::string &walletsResponse);

    void shouldVerifySigning(const UniValue &paymentProposal, int actionType, const QString& signature);
    void signedProposalAvailable(const UniValue& proposal, const std::vector<std::string> &vSigs);

private:
    Ui::MainWindow* ui;
    BackupDialog *backupDialog;
    QLabel* statusBarLabelLeft;
    QLabel* statusBarLabelRight;
    QPushButton* statusBarButton;
    bool processComnand;
    bool deviceConnected;
    bool cachedWalletAvailableState;
    QPropertyAnimation *loginScreenIndicatorOpacityAnimation;
    QPropertyAnimation *statusBarloadingIndicatorOpacityAnimation;
    std::string sessionPassword; //TODO: needs secure space / mem locking
    std::string sessionPasswordDuringChangeProcess; //TODO: needs secure space / mem locking
    std::vector<DBBMultisigWallet> vMultisigWallets;

    //!check device state and do a UI update
    void checkDevice();

    bool sendCommand(const std::string& cmd, const std::string& password, dbb_response_type_t tag = DBB_RESPONSE_TYPE_UNKNOWN);
    void _JoinCopayWallet(int walletIndex);
    bool QTexecuteCommandWrapper(const std::string& cmd, const dbb_process_infolayer_style_t layerstyle, std::function<void(const std::string&, dbb_cmd_execution_status_t status)> cmdFinished);

    QAction *overviewAction;
    QAction *walletsAction;
    QAction *settingsAction;

    void setActiveArrow(int pos);
    void hidePaymentProposalsWidget();
    PaymentProposal *currentPaymentProposalWidget;
    void hideSessionPasswordView();
    void setTabbarEnabled(bool status);
    void passwordAccepted();

    bool netThreadBusy;
    std::thread netThread;
    std::mutex cs_vMultisigWallets;
};

#endif
