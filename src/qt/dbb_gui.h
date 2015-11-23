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
#include <QStandardItemModel>

#include <functional>
#include <thread>
#include <mutex>

#include "dbb_app.h"
#include "dbb_wallet.h"
#include "dbb_websocketserver.h"

#include "backupdialog.h"
#include "paymentproposal.h"
#include "signconfirmationdialog.h"

#define WEBSOCKET_PORT 25698

class BonjourServiceRegister;

namespace Ui
{
class MainWindow;
}

//DBB USB response types
typedef enum DBB_RESPONSE_TYPE {
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
    DBB_RESPONSE_TYPE_DEVICE_LOCK,
    DBB_RESPONSE_TYPE_VERIFYPASS_ECDH
} dbb_response_type_t;

typedef enum DBB_PROCESS_INFOLAYER_STYLE {
    DBB_PROCESS_INFOLAYER_STYLE_NO_INFO,
    DBB_PROCESS_INFOLAYER_STYLE_TOUCHBUTTON
} dbb_process_infolayer_style_t;

class DBBDaemonGui : public QMainWindow
{
    Q_OBJECT

public:
    explicit DBBDaemonGui(QWidget* parent = 0);
    ~DBBDaemonGui();

public slots:

signals:
    //emited when the device state has chaned (connected / disconnected)
    void deviceStateHasChanged(bool state);
    //emited when the DBB could generate a xpub
    void XPubForCopayWalletIsAvailable(int walletIndex);
    //emited when the request xpub key is available
    void RequestXPubKeyForCopayWalletIsAvailable(int walletIndex);
    //emited when a response from the DBB is available
    void gotResponse(const UniValue& response, dbb_cmd_execution_status_t status, dbb_response_type_t tag, int subtag = 0);
    //emited when a copay getwallet response is available
    void getWalletsResponseAvailable(DBBWallet* wallet, bool walletsAvailable, const std::string& walletsResponse);
    //emited when a copay wallet history response is available
    void getTransactionHistoryAvailable(DBBWallet* wallet, bool historyAvailable, const UniValue& historyResponse);
    //emited when a payment proposal and a given signatures should be verified
    void shouldVerifySigning(DBBWallet*, const UniValue& paymentProposal, int actionType, const std::string& signature);
    //emited when the verification dialog shoud hide
    void shouldHideVerificationInfo();
    //emited when signatures for a payment proposal are available
    void signedProposalAvailable(DBBWallet*, const UniValue& proposal, const std::vector<std::string>& vSigs);
    //emited when a wallet needs update
    void shouldUpdateWallet(DBBWallet*);
    //emited when a new receiving address is available
    void walletAddressIsAvailable(DBBWallet *, const std::string &newAddress, const std::string &keypath);
    //emited when a new receiving address is available
    void paymentProposalUpdated(DBBWallet *, const UniValue &proposal);

private:
    Ui::MainWindow* ui;
    BackupDialog* backupDialog;
    WebsocketServer *websocketServer;
    BonjourServiceRegister *bonjourRegister;
    QStandardItemModel *transactionTableModel;
    QLabel* statusBarLabelLeft;
    QLabel* statusBarLabelRight;
    QPushButton* statusBarButton;
    QAction* overviewAction;
    QAction* walletsAction;
    QAction* settingsAction;
    bool sdcardWarned;
    bool processCommand;
    bool deviceConnected;
    bool cachedWalletAvailableState;
    bool deviceReadyToInteract;
    QPropertyAnimation* loginScreenIndicatorOpacityAnimation;
    QPropertyAnimation* statusBarloadingIndicatorOpacityAnimation;
    std::string sessionPassword;                    //TODO: needs secure space / mem locking
    std::string sessionPasswordDuringChangeProcess; //TODO: needs secure space / mem locking
    std::vector<DBBWallet*> vMultisigWallets;       //!<immutable pointers to the multisig wallet objects (currently only 1)
    DBBWallet* singleWallet;                        //!<immutable pointer to the single wallet object
    PaymentProposal* currentPaymentProposalWidget;  //!< UI element for showing a payment proposal
    SignConfirmationDialog* signConfirmationDialog;  //!< UI element for showing a payment proposal


    //== Plug / Unplug ==
    //! gets called when the device was sucessfully unlocked (password accepted)
    void deviceIsReadyToInteract();
    //!enabled or disabled the tabbar
    void setTabbarEnabled(bool status);
    //!enable or disable dbb/usb/loading indication in the UI
    void setLoading(bool status);
    //!enable or disable net/loading indication in the UI
    void setNetLoading(bool status);
    //!resets device infos (in case of a disconnect)
    void resetInfos();
    //! updates the ui after the current device state
    void uiUpdateDeviceState();

    //== UI ==
    //general proxy function to show an alert;
    void showAlert(const QString& title, const QString& errorOut, bool critical = false);
    void setActiveArrow(int pos);
    //!gets called when the password was accepted by the DBB hardware
    void passwordAccepted();
    //!hides the enter password form
    void hideSessionPasswordView();
    //!show general modal info
    void showModalInfo(const QString &info);
    void hideModalInfo();
    //!update overview flags (wallet / lock, etc)
    void updateOverviewFlags(bool walletAvailable, bool lockAvailable, bool loading);

    //== USB ==
    //wrapper for the DBB USB command action
    bool executeCommandWrapper(const std::string& cmd, const dbb_process_infolayer_style_t layerstyle, std::function<void(const std::string&, dbb_cmd_execution_status_t status)> cmdFinished);

    //== Copay Wallet ==
    void hidePaymentProposalsWidget();
    void executeNetUpdateWallet(DBBWallet* wallet, std::function<void(bool, std::string&)> cmdFinished);


    bool multisigWalletIsUpdating;
    bool singleWalletIsUpdating;
    std::vector<std::pair<std::time_t, std::thread*> > netThreads;
    std::mutex cs_vMultisigWallets;

private slots:
    //!main callback when the device gets connected/disconnected
    void changeConnectedState(bool state);

    //== UI ==
    //general proxy function to show an alert;
    void mainOverviewButtonClicked();
    void mainMultisigButtonClicked();
    void mainReceiveButtonClicked();
    void mainSendButtonClicked();
    void mainSettingsButtonClicked();
    void gotoOverviewPage();
    void gotoReceivePage();
    void gotoSendCoinsPage();
    void gotoMultisigPage();
    void gotoSettingsPage();
    //!shows info about the smartphone verification
    void showEchoVerification(DBBWallet*, const UniValue& response, int actionType, const std::string& echoStr);
    //!hides verification info
    void hideVerificationInfo();
    //!gets called when the user hits enter in the "enter password form"
    void passwordProvided();
    //!slot to ask for the current session password
    void askForSessionPassword();

    //== DBB USB ==
    //!function is user whishes to erase the DBB
    void eraseClicked();
    void ledClicked();
    //!get basic informations about the connected DBB
    void getInfo();
    void setPasswordClicked(bool showInfo = true);
    //!seed the wallet, flush everything and create a new bip32 entropy
    void seedHardware();

    //== DBB USB / UTILS ==
    //!get a random number from the dbb
    void getRandomNumber();
    //!lock the device, disabled "backup", "verifypass" and "seed" command
    void lockDevice();

    //== DBB USB / BACKUP ==
    void showBackupDialog();
    void addBackup();
    void listBackup();
    void eraseAllBackups();
    void restoreBackup(const QString& backupFilename);

    //== DBB USB Commands (Response Parsing) ==
    //!main function to parse a response from the DBB
    void parseResponse(const UniValue& response, dbb_cmd_execution_status_t status, dbb_response_type_t tag, int subtag);

    //== Copay Wallet ==
    //!create a single wallet
    void createSingleWallet();
    //!get a new address
    void getNewAddress();
    //!gets called when a new address is available
    void updateReceivingAddress(DBBWallet *wallet, const std::string &newAddress, const std::string &keypath);
    //!check the UI values and create a payment proposal from them, sign and post them
    void createTxProposalPressed();
    //!Report about a submitted payment proposal
    void reportPaymentProposalPost(DBBWallet* wallet, const UniValue& proposal);
    void joinCopayWalletClicked();
    //!initiates a copay multisig wallet join
    void joinMultisigWalletInitiate(DBBWallet*);
    //!gets a xpub key for the copay wallet
    void getXPubKeyForCopay(int walletIndex);
    //!gets a xpub key at the keypath that is used for the request private key
    void getRequestXPubKeyForCopay(int walletIndex);
    //!joins a copay wallet with given xpub key
    void joinCopayWalletWithXPubKey(int walletIndex);
    //!update a given wallet object
    void updateWallet(DBBWallet* wallet);
    //!update multisig wallets
    void MultisigUpdateWallets();
    //!update single wallet
    void SingleWalletUpdateWallets();
    //!update the mutisig ui from a getWallets response
    void updateUIMultisigWallets(const UniValue& walletResponse);
    //!update the singlewallet ui from a getWallets response
    void updateUISingleWallet(const UniValue& walletResponse);
    //!update the single wallet transaction table
    void updateTransactionTable(DBBWallet *wallet, bool historyAvailable, const UniValue& history);

    //!parse single wallet wallet response
    void parseWalletsResponse(DBBWallet* wallet, bool walletsAvailable, const std::string& walletsResponse);
    //!update payment proposals
    bool MultisigUpdatePaymentProposals(const UniValue& response);
    //!show a single payment proposals with given id
    bool MultisigShowPaymentProposal(const UniValue& pendingTxps, const std::string& targetID);
    //!execute payment proposal action
    void PaymentProposalAction(DBBWallet* wallet, const UniValue& paymentProposal, int actionType);
    //!post
    void postSignaturesForPaymentProposal(DBBWallet* wallet, const UniValue& proposal, const std::vector<std::string>& vSigs);

    //== ECDH Pairing ==
    //!send a ecdh pairing request with pubkey to the DBB
    void sendECDHPairingRequest(const std::string &pubkey);
};

#endif
