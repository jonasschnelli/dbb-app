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
#include <QTimer>
#include <QToolBar>
#include <QFontDatabase>
#include <QGraphicsOpacityEffect>
#include <QtNetwork/QHostInfo>

#include "ui/ui_overview.h"
#include <dbb.h>

#include "dbb_util.h"
#include "dbb_netthread.h"
#include "bonjourserviceregister.h"

#include <cstdio>
#include <ctime>
#include <chrono>

#include <univalue.h>
#include <btc/bip32.h>
#include <btc/tx.h>

#include <qrencode.h>

//function from dbb_app.cpp
extern void executeCommand(const std::string& cmd, const std::string& password, std::function<void(const std::string&, dbb_cmd_execution_status_t status)> cmdFinished);


DBBDaemonGui::~DBBDaemonGui()
{
    delete bonjourRegister;
}

void testfunc(DNSServiceRef, DNSServiceFlags,
              DNSServiceErrorType errorCode, const char *name,
              const char *regtype, const char *domain,
              void *data)
{

}

DBBDaemonGui::DBBDaemonGui(QWidget* parent) : QMainWindow(parent),
                                              ui(new Ui::MainWindow),
                                              overviewAction(0),
                                              walletsAction(0),
                                              settingsAction(0),
                                              statusBarButton(0),
                                              statusBarLabelRight(0),
                                              statusBarLabelLeft(0),
                                              backupDialog(0),
                                              getAddressDialog(0),
                                              websocketServer(0),
                                              processCommand(0),
                                              deviceConnected(0),
                                              deviceReadyToInteract(0),
                                              cachedWalletAvailableState(0),
                                              currentPaymentProposalWidget(0),
                                              signConfirmationDialog(0),
                                              loginScreenIndicatorOpacityAnimation(0),
                                              statusBarloadingIndicatorOpacityAnimation(0),
                                              sdcardWarned(0)
{
    QApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);

    ui->setupUi(this);

/////////// UI Styling
#if defined(Q_OS_MAC)
    std::string balanceFontSize = "24pt";
    std::string menuFontSize = "18pt";
    std::string stdFontSize = "16pt";
    std::string smallFontSize = "12pt";
#elif defined(Q_OS_WIN)
    std::string balanceFontSize = "24pt";
    std::string menuFontSize = "18pt";
    std::string stdFontSize = "16pt";
    std::string smallFontSize = "12pt";
#else
    std::string balanceFontSize = "20pt";
    std::string menuFontSize = "14pt";
    std::string stdFontSize = "12pt";
    std::string smallFontSize = "10pt";
#endif

    QFontDatabase::addApplicationFont(":/fonts/AlegreyaSans-Regular");
    QFontDatabase::addApplicationFont(":/fonts/AlegreyaSans-Bold");

    qApp->setStyleSheet("QWidget { font-family: Alegreya Sans; font-size:" + QString::fromStdString(stdFontSize) + "; }");
    this->setStyleSheet("DBBDaemonGui { background-image: url(:/theme/windowbackground);;  } QToolBar { background-color: white }");
    QString buttonCss("QPushButton::hover { } QPushButton:pressed { background-color: #444444; border:0; color: white; } QPushButton { font-family: Alegreya Sans; font-weight: bold; font-size:" + QString::fromStdString(menuFontSize) + "; background-color: black; border:0; color: white; };");
    QString msButtonCss("QPushButton::hover { } QPushButton:pressed { background-color: #444444; border:0; color: white; } QPushButton { font-family: Alegreya Sans; font-weight: bold; font-size:" + QString::fromStdString(menuFontSize) + "; background-color: #003366; border:0; color: white; };");

    QString labelCSS("QLabel { font-size: " + QString::fromStdString(smallFontSize) + "; }");

    this->ui->receiveButton->setStyleSheet(buttonCss);
    this->ui->overviewButton->setStyleSheet(buttonCss);
    this->ui->sendButton->setStyleSheet(buttonCss);
    this->ui->mainSettingsButton->setStyleSheet(buttonCss);
    this->ui->multisigButton->setStyleSheet(msButtonCss);

    this->ui->deviceNameKeyLabel->setStyleSheet(labelCSS);
    this->ui->deviceNameLabel->setStyleSheet(labelCSS);
    this->ui->versionKeyLabel->setStyleSheet(labelCSS);
    this->ui->versionLabel->setStyleSheet(labelCSS);
    this->ui->keypathLabel->setStyleSheet(labelCSS);
    


    this->ui->balanceLabel->setStyleSheet("font-size: " + QString::fromStdString(balanceFontSize) + ";");
    this->ui->singleWalletBalance->setStyleSheet("font-size: " + QString::fromStdString(balanceFontSize) + ";");
    this->ui->multisigBalance->setStyleSheet("font-size: " + QString::fromStdString(balanceFontSize) + ";");

    this->ui->multisigBalanceKey->setStyleSheet(labelCSS);
    this->ui->multisigWalletNameKey->setStyleSheet(labelCSS);
    ////////////// END STYLING

    ui->touchbuttonInfo->setVisible(false);
    // set light transparent background for touch button info layer
    this->ui->touchbuttonInfo->setStyleSheet("background-color: rgba(255, 255, 255, 240);");

    // allow serval signaling data types
    qRegisterMetaType<UniValue>("UniValue");
    qRegisterMetaType<std::string>("std::string");
    qRegisterMetaType<dbb_cmd_execution_status_t>("dbb_cmd_execution_status_t");
    qRegisterMetaType<dbb_response_type_t>("dbb_response_type_t");
    qRegisterMetaType<std::vector<std::string> >("std::vector<std::string>");
    qRegisterMetaType<DBBWallet*>("DBBWallet *");

    // connect UI
    connect(ui->eraseButton, SIGNAL(clicked()), this, SLOT(eraseClicked()));
    connect(ui->ledButton, SIGNAL(clicked()), this, SLOT(ledClicked()));
    connect(ui->passwordButton, SIGNAL(clicked()), this, SLOT(setPasswordClicked()));
    connect(ui->seedButton, SIGNAL(clicked()), this, SLOT(seedHardware()));
    connect(ui->createSingleWallet, SIGNAL(clicked()), this, SLOT(createSingleWallet()));
    connect(ui->getNewAddress, SIGNAL(clicked()), this, SLOT(getNewAddress()));
    connect(ui->verifyAddressButton, SIGNAL(clicked()), this, SLOT(verifyAddress()));
    connect(ui->joinCopayWallet, SIGNAL(clicked()), this, SLOT(joinCopayWalletClicked()));
    connect(ui->checkProposals, SIGNAL(clicked()), this, SLOT(MultisigUpdateWallets()));
    connect(ui->showBackups, SIGNAL(clicked()), this, SLOT(showBackupDialog()));
    connect(ui->getRand, SIGNAL(clicked()), this, SLOT(getRandomNumber()));
    connect(ui->lockDevice, SIGNAL(clicked()), this, SLOT(lockDevice()));
    connect(ui->sendCoinsButton, SIGNAL(clicked()), this, SLOT(createTxProposalPressed()));
    connect(ui->getAddress, SIGNAL(clicked()), this, SLOT(showGetAddressDialog()));


    // connect custom signals
    connect(this, SIGNAL(XPubForCopayWalletIsAvailable(int)), this, SLOT(getRequestXPubKeyForCopay(int)));
    connect(this, SIGNAL(RequestXPubKeyForCopayWalletIsAvailable(int)), this, SLOT(joinCopayWalletWithXPubKey(int)));
    connect(this, SIGNAL(gotResponse(const UniValue&, dbb_cmd_execution_status_t, dbb_response_type_t, int)), this, SLOT(parseResponse(const UniValue&, dbb_cmd_execution_status_t, dbb_response_type_t, int)));
    connect(this, SIGNAL(shouldVerifySigning(DBBWallet*, const UniValue&, int, const std::string&)), this, SLOT(showEchoVerification(DBBWallet*, const UniValue&, int, const std::string&)));
    connect(this, SIGNAL(shouldHideVerificationInfo()), this, SLOT(hideVerificationInfo()));
    connect(this, SIGNAL(signedProposalAvailable(DBBWallet*, const UniValue&, const std::vector<std::string>&)), this, SLOT(postSignaturesForPaymentProposal(DBBWallet*, const UniValue&, const std::vector<std::string>&)));
    connect(this, SIGNAL(getWalletsResponseAvailable(DBBWallet*, bool, const std::string&)), this, SLOT(parseWalletsResponse(DBBWallet*, bool, const std::string&)));
    connect(this, SIGNAL(getTransactionHistoryAvailable(DBBWallet*, bool, const UniValue&)), this, SLOT(updateTransactionTable(DBBWallet*, bool, const UniValue&)));

    connect(this, SIGNAL(shouldUpdateWallet(DBBWallet*)), this, SLOT(updateWallet(DBBWallet*)));
    connect(this, SIGNAL(walletAddressIsAvailable(DBBWallet*,const std::string &,const std::string &)), this, SLOT(updateReceivingAddress(DBBWallet*,const std::string&,const std::string &)));
    connect(this, SIGNAL(paymentProposalUpdated(DBBWallet*,const UniValue&)), this, SLOT(reportPaymentProposalPost(DBBWallet*,const UniValue&)));


    // create backup dialog instance
    backupDialog = new BackupDialog(0);
    connect(backupDialog, SIGNAL(addBackup()), this, SLOT(addBackup()));
    connect(backupDialog, SIGNAL(eraseAllBackups()), this, SLOT(eraseAllBackups()));
    connect(backupDialog, SIGNAL(restoreFromBackup(const QString&)), this, SLOT(restoreBackup(const QString&)));

    // create get address dialog
    getAddressDialog = new GetAddressDialog(0);
    connect(getAddressDialog, SIGNAL(shouldGetXPub(const QString&)), this, SLOT(getAddressGetXPub(const QString&)));

    //set window icon
    QApplication::setWindowIcon(QIcon(":/icons/dbb"));
    setWindowTitle(tr("The Digital Bitbox"));

    //set status bar
    QWidget* spacer = new QWidget();
    spacer->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    spacer->setMinimumWidth(3);
    spacer->setMaximumHeight(10);
    statusBar()->addWidget(spacer);
    statusBar()->setStyleSheet("background: transparent;");
    this->statusBarButton = new QPushButton(QIcon(":/icons/connected"), "");
    this->statusBarButton->setEnabled(false);
    this->statusBarButton->setFlat(true);
    this->statusBarButton->setMaximumWidth(18);
    this->statusBarButton->setMaximumHeight(18);
    this->statusBarButton->setVisible(false);
    statusBar()->addWidget(this->statusBarButton);

    this->statusBarLabelLeft = new QLabel(tr("No Device Found"));
    statusBar()->addWidget(this->statusBarLabelLeft);

    this->statusBarLabelRight = new QLabel("");
    statusBar()->addPermanentWidget(this->statusBarLabelRight);
    if (!statusBarloadingIndicatorOpacityAnimation) {
        QGraphicsOpacityEffect* eff = new QGraphicsOpacityEffect(this);
        this->statusBarLabelRight->setGraphicsEffect(eff);

        statusBarloadingIndicatorOpacityAnimation = new QPropertyAnimation(eff, "opacity");

        statusBarloadingIndicatorOpacityAnimation->setDuration(500);
        statusBarloadingIndicatorOpacityAnimation->setKeyValueAt(0, 0.3);
        statusBarloadingIndicatorOpacityAnimation->setKeyValueAt(0.5, 1.0);
        statusBarloadingIndicatorOpacityAnimation->setKeyValueAt(1, 0.3);
        statusBarloadingIndicatorOpacityAnimation->setEasingCurve(QEasingCurve::OutQuad);
        statusBarloadingIndicatorOpacityAnimation->setLoopCount(-1);
    }


    // tabbar
    QActionGroup* tabGroup = new QActionGroup(this);
    overviewAction = new QAction(QIcon(":/icons/home").pixmap(32), tr("&Overview"), this);
    overviewAction->setToolTip(tr("Show general overview of wallet"));
    overviewAction->setCheckable(true);
    overviewAction->setShortcut(QKeySequence(Qt::ALT + Qt::Key_1));
    tabGroup->addAction(overviewAction);

    walletsAction = new QAction(QIcon(":/icons/copay"), tr("&Wallets"), this);
    walletsAction->setToolTip(tr("Show Copay wallet screen"));
    walletsAction->setCheckable(true);
    walletsAction->setShortcut(QKeySequence(Qt::ALT + Qt::Key_2));
    tabGroup->addAction(walletsAction);

    settingsAction = new QAction(QIcon(":/icons/settings"), tr("&Experts"), this);
    settingsAction->setToolTip(tr("Show Settings wallet screen"));
    settingsAction->setCheckable(true);
    settingsAction->setShortcut(QKeySequence(Qt::ALT + Qt::Key_3));
    tabGroup->addAction(settingsAction);

    connect(this->ui->overviewButton, SIGNAL(clicked()), this, SLOT(mainOverviewButtonClicked()));
    connect(this->ui->multisigButton, SIGNAL(clicked()), this, SLOT(mainMultisigButtonClicked()));
    connect(this->ui->receiveButton, SIGNAL(clicked()), this, SLOT(mainReceiveButtonClicked()));
    connect(this->ui->sendButton, SIGNAL(clicked()), this, SLOT(mainSendButtonClicked()));
    connect(this->ui->mainSettingsButton, SIGNAL(clicked()), this, SLOT(mainSettingsButtonClicked()));

    connect(overviewAction, SIGNAL(triggered()), this, SLOT(gotoOverviewPage()));
    connect(walletsAction, SIGNAL(triggered()), this, SLOT(gotoMultisigPage()));
    connect(settingsAction, SIGNAL(triggered()), this, SLOT(gotoSettingsPage()));

    //login screen setup
    this->ui->blockerView->setVisible(false);
    connect(this->ui->passwordLineEdit, SIGNAL(returnPressed()), this, SLOT(passwordProvided()));

    //load local pubkeys
    singleWallet = new DBBWallet();
    singleWallet->client.setFilenameBase("copay_single");
    singleWallet->baseKeyPath = "m/203'";
    singleWallet->client.LoadLocalData();
    std::string lastAddress, keypath;
    singleWallet->client.GetLastKnownAddress(lastAddress, keypath);
    singleWallet->rewriteKeypath(keypath);
    updateReceivingAddress(singleWallet, lastAddress, keypath);

    DBBWallet* copayWallet = new DBBWallet();
    copayWallet->client.setFilenameBase("copay_multisig");
    copayWallet->client.LoadLocalData();
    vMultisigWallets.push_back(copayWallet);


    deviceConnected = false;
    resetInfos();
    //set status bar connection status
    uiUpdateDeviceState();
    changeConnectedState(DBB::isConnectionOpen());


    processCommand = false;

    //connect the device status update at very last point in init
    connect(this, SIGNAL(deviceStateHasChanged(bool)), this, SLOT(changeConnectedState(bool)));

    //create a local websocket server
    websocketServer = new WebsocketServer(WEBSOCKET_PORT, true);
    connect(websocketServer, SIGNAL(ecdhPairingRequest(const std::string&)), this, SLOT(sendECDHPairingRequest(const std::string&)));

    //announce service over mDNS
    bonjourRegister = new BonjourServiceRegister(this);
    bonjourRegister->registerService(BonjourRecord(tr("Digital Bitbox App Websocket"), QLatin1String("_dbb._tcp."), QString()), WEBSOCKET_PORT);
}

/*
 /////////////////////////////
 Plug / Unplug / GetInfo stack
 /////////////////////////////
*/
#pragma mark plug / unpluag stack

void DBBDaemonGui::deviceIsReadyToInteract()
{
    //update multisig wallet data
    MultisigUpdateWallets();
    SingleWalletUpdateWallets();
    deviceReadyToInteract = true;
}

void DBBDaemonGui::changeConnectedState(bool state)
{
    bool stateChanged = deviceConnected != state;
    if (stateChanged) {
        if (state) {
            deviceConnected = true;
            this->statusBarLabelLeft->setText("Device Connected");
            this->statusBarButton->setVisible(true);
        } else {
            deviceConnected = false;
            this->statusBarLabelLeft->setText("No Device Found");
            this->statusBarButton->setVisible(false);
        }

        uiUpdateDeviceState();
    }
}

void DBBDaemonGui::setTabbarEnabled(bool status)
{
    this->ui->menuBlocker->setVisible(!status);
    this->ui->overviewButton->setEnabled(status);
    this->ui->receiveButton->setEnabled(status);
    this->ui->sendButton->setEnabled(status);
    this->ui->mainSettingsButton->setEnabled(status);
    this->ui->multisigButton->setEnabled(status);
}

void DBBDaemonGui::setLoading(bool status)
{
    if (!status)
        ui->touchbuttonInfo->setVisible(false);

    this->statusBarLabelRight->setText((status) ? "processing..." : "");

    if (statusBarloadingIndicatorOpacityAnimation) {
        if (status)
            statusBarloadingIndicatorOpacityAnimation->start(QAbstractAnimation::KeepWhenStopped);
        else
            statusBarloadingIndicatorOpacityAnimation->stop();
    }

    this->ui->unlockingInfo->setText((status) ? "Unlocking Device..." : "");
    //TODO, subclass label and make it animated
}

void DBBDaemonGui::setNetLoading(bool status)
{
    this->statusBarLabelRight->setText((status) ? "loading..." : "");

    if (statusBarloadingIndicatorOpacityAnimation) {
        if (status)
            statusBarloadingIndicatorOpacityAnimation->start(QAbstractAnimation::KeepWhenStopped);
        else
            statusBarloadingIndicatorOpacityAnimation->stop();
    }
}

void DBBDaemonGui::resetInfos()
{
    this->ui->versionLabel->setText("loading info...");
    this->ui->deviceNameLabel->setText("loading info...");

    updateOverviewFlags(false, false, true);
}

void DBBDaemonGui::uiUpdateDeviceState()
{
    this->ui->verticalLayoutWidget->setVisible(deviceConnected);
    this->ui->balanceLabel->setVisible(deviceConnected);
    this->ui->noDeviceWidget->setVisible(!deviceConnected);

    if (!deviceConnected) {
        walletsAction->setEnabled(false);
        settingsAction->setEnabled(false);
        gotoOverviewPage();
        setActiveArrow(0);
        overviewAction->setChecked(true);
        resetInfos();
        sessionPassword.clear();
        hideSessionPasswordView();
        setTabbarEnabled(false);
        deviceReadyToInteract = false;
        //hide modal dialog and abort possible ecdh pairing
        hideModalInfo();
        if (websocketServer)
            websocketServer->abortECDHPairing();

        //clear some infos
        ui->tableWidget->setModel(NULL);
        this->ui->balanceLabel->setText("");
        this->ui->singleWalletBalance->setText("");
        sdcardWarned = false;

    } else {
        walletsAction->setEnabled(true);
        settingsAction->setEnabled(true);
        askForSessionPassword();
    }
}

/*
 /////////////////
 UI Action Stack
 /////////////////
*/
#pragma mark UI Action Stack

void DBBDaemonGui::showAlert(const QString& title, const QString& errorOut, bool critical)
{
    if (critical)
        QMessageBox::critical(this, title, errorOut, QMessageBox::Ok);
    else
        QMessageBox::warning(this, title, errorOut, QMessageBox::Ok);
}

void DBBDaemonGui::setActiveArrow(int pos)
{
    this->ui->activeArrow->move(pos * 96 + 40, 39);
}

void DBBDaemonGui::mainOverviewButtonClicked()
{
    setActiveArrow(0);
    gotoOverviewPage();
}

void DBBDaemonGui::mainMultisigButtonClicked()
{
    setActiveArrow(4);
    gotoMultisigPage();
}

void DBBDaemonGui::mainReceiveButtonClicked()
{
    setActiveArrow(1);
    gotoReceivePage();
}

void DBBDaemonGui::mainSendButtonClicked()
{
    setActiveArrow(2);
    gotoSendCoinsPage();
}

void DBBDaemonGui::mainSettingsButtonClicked()
{
    setActiveArrow(3);
    gotoSettingsPage();
}

void DBBDaemonGui::gotoOverviewPage()
{
    this->ui->stackedWidget->setCurrentIndex(0);
}

void DBBDaemonGui::gotoSendCoinsPage()
{
    this->ui->stackedWidget->setCurrentIndex(3);
}

void DBBDaemonGui::gotoReceivePage()
{
    this->ui->stackedWidget->setCurrentIndex(4);
}

void DBBDaemonGui::gotoMultisigPage()
{
    this->ui->stackedWidget->setCurrentIndex(1);
}

void DBBDaemonGui::gotoSettingsPage()
{
    this->ui->stackedWidget->setCurrentIndex(2);
}

void DBBDaemonGui::showEchoVerification(DBBWallet* wallet, const UniValue& proposalData, int actionType, const std::string& echoStr)
{
    if (!signConfirmationDialog) {
        signConfirmationDialog = new SignConfirmationDialog(0);
    }

    if (websocketServer)
        websocketServer->sendStringToAllClients(echoStr);

    signConfirmationDialog->setData(proposalData);
    signConfirmationDialog->show();
    PaymentProposalAction(wallet, proposalData, actionType);
}

void DBBDaemonGui::hideVerificationInfo()
{
    if (signConfirmationDialog) {
        signConfirmationDialog->hide();
    }
}


void DBBDaemonGui::passwordProvided()
{
    if (!loginScreenIndicatorOpacityAnimation) {
        QGraphicsOpacityEffect* eff = new QGraphicsOpacityEffect(this);
        this->ui->unlockingInfo->setGraphicsEffect(eff);

        loginScreenIndicatorOpacityAnimation = new QPropertyAnimation(eff, "opacity");

        loginScreenIndicatorOpacityAnimation->setDuration(500);
        loginScreenIndicatorOpacityAnimation->setKeyValueAt(0, 0.3);
        loginScreenIndicatorOpacityAnimation->setKeyValueAt(0.5, 1.0);
        loginScreenIndicatorOpacityAnimation->setKeyValueAt(1, 0.3);
        loginScreenIndicatorOpacityAnimation->setEasingCurve(QEasingCurve::OutQuad);
        loginScreenIndicatorOpacityAnimation->setLoopCount(-1);
    }

    // to slide in call
    loginScreenIndicatorOpacityAnimation->start(QAbstractAnimation::KeepWhenStopped);

    sessionPassword = this->ui->passwordLineEdit->text().toStdString();
    getInfo();
}

void DBBDaemonGui::passwordAccepted()
{
    hideSessionPasswordView();
    this->ui->passwordLineEdit->setText("");
    setTabbarEnabled(true);
}

void DBBDaemonGui::askForSessionPassword()
{
    setLoading(false);
    this->ui->blockerView->setVisible(true);
    QWidget* slide = this->ui->blockerView;
    // setup slide
    slide->setGeometry(-slide->width(), 0, slide->width(), slide->height());

    // then a animation:
    QPropertyAnimation* animation = new QPropertyAnimation(slide, "pos");
    animation->setDuration(300);
    animation->setStartValue(slide->pos());
    animation->setEndValue(QPoint(0, 0));
    animation->setEasingCurve(QEasingCurve::OutQuad);
    // to slide in call
    animation->start(QAbstractAnimation::DeleteWhenStopped);
    ui->passwordLineEdit->setFocus();
}

void DBBDaemonGui::hideSessionPasswordView()
{
    if (loginScreenIndicatorOpacityAnimation)
        loginScreenIndicatorOpacityAnimation->stop();

    QWidget* slide = this->ui->blockerView;

    // then a animation:
    QPropertyAnimation* animation = new QPropertyAnimation(slide, "pos");
    animation->setDuration(300);
    animation->setStartValue(slide->pos());
    animation->setEndValue(QPoint(-slide->width(), 0));
    animation->setEasingCurve(QEasingCurve::OutQuad);
    // to slide in call
    animation->start(QAbstractAnimation::DeleteWhenStopped);
}

void DBBDaemonGui::showModalInfo(const QString &info)
{
    ui->modalInfoLabel->setText(info);
    this->ui->modalBlockerView->setVisible(true);
    QWidget* slide = this->ui->modalBlockerView;
    // setup slide
    slide->setGeometry(-slide->width(), 0, slide->width(), slide->height());

    // then a animation:
    QPropertyAnimation* animation = new QPropertyAnimation(slide, "pos");
    animation->setDuration(300);
    animation->setStartValue(slide->pos());
    animation->setEndValue(QPoint(0, 0));
    animation->setEasingCurve(QEasingCurve::OutQuad);
    // to slide in call
    animation->start(QAbstractAnimation::DeleteWhenStopped);
}

void DBBDaemonGui::hideModalInfo()
{
    QWidget* slide = this->ui->modalBlockerView;

    // then a animation:
    QPropertyAnimation* animation = new QPropertyAnimation(slide, "pos");
    animation->setDuration(300);
    animation->setStartValue(slide->pos());
    animation->setEndValue(QPoint(-slide->width(), 0));
    animation->setEasingCurve(QEasingCurve::OutQuad);
    // to slide in call
    animation->start(QAbstractAnimation::DeleteWhenStopped);
}

void DBBDaemonGui::updateOverviewFlags(bool walletAvailable, bool lockAvailable, bool loading)
{
//    this->ui->walletCheckmark->setIcon(QIcon(walletAvailable ? ":/icons/okay" : ":/icons/warning"));
//    this->ui->walletLabel->setText(tr(walletAvailable ? "Wallet available" : "No Wallet"));
//    this->ui->createWallet->setVisible(!walletAvailable);
//
//    this->ui->lockCheckmark->setIcon(QIcon(lockAvailable ? ":/icons/okay" : ":/icons/warning"));
//    this->ui->lockLabel->setText(lockAvailable ? "Device 2FA Lock" : "No 2FA set");
//
//    if (loading) {
//        this->ui->lockLabel->setText("loading info...");
//        this->ui->walletLabel->setText("loading info...");
//
//        this->ui->walletCheckmark->setIcon(QIcon(":/icons/warning")); //TODO change to loading...
//        this->ui->lockCheckmark->setIcon(QIcon(":/icons/warning"));   //TODO change to loading...
//    }
}

/*
 //////////////////////////
 DBB USB Commands (General)
 //////////////////////////
*/
#pragma mark DBB USB Commands (General)

bool DBBDaemonGui::executeCommandWrapper(const std::string& cmd, const dbb_process_infolayer_style_t layerstyle, std::function<void(const std::string&, dbb_cmd_execution_status_t status)> cmdFinished)
{
    if (processCommand)
        return false;

    if (layerstyle == DBB_PROCESS_INFOLAYER_STYLE_TOUCHBUTTON) {
        ui->touchbuttonInfo->setVisible(true);
    }

    setLoading(true);
    processCommand = true;
    executeCommand(cmd, sessionPassword, cmdFinished);

    return true;
}

void DBBDaemonGui::eraseClicked()
{
    if (executeCommandWrapper("{\"reset\":\"__ERASE__\"}", DBB_PROCESS_INFOLAYER_STYLE_TOUCHBUTTON, [this](const std::string& cmdOut, dbb_cmd_execution_status_t status) {
            UniValue jsonOut;
            jsonOut.read(cmdOut);
            emit gotResponse(jsonOut, status, DBB_RESPONSE_TYPE_ERASE);
        })) {
        std::unique_lock<std::mutex> lock(this->cs_vMultisigWallets);

        singleWallet->client.RemoveLocalData();
        vMultisigWallets[0]->client.RemoveLocalData();

        sessionPasswordDuringChangeProcess = sessionPassword;
        sessionPassword.clear();
    }
}

void DBBDaemonGui::ledClicked()
{
    executeCommandWrapper("{\"led\" : \"toggle\"}", DBB_PROCESS_INFOLAYER_STYLE_NO_INFO, [this](const std::string& cmdOut, dbb_cmd_execution_status_t status) {
        UniValue jsonOut;
        jsonOut.read(cmdOut);

        emit gotResponse(jsonOut, status, DBB_RESPONSE_TYPE_LED_BLINK);
    });
}

void DBBDaemonGui::getInfo()
{
    executeCommandWrapper("{\"device\":\"info\"}", DBB_PROCESS_INFOLAYER_STYLE_NO_INFO, [this](const std::string& cmdOut, dbb_cmd_execution_status_t status) {
        UniValue jsonOut;
        jsonOut.read(cmdOut);
        emit gotResponse(jsonOut, status, DBB_RESPONSE_TYPE_INFO);
    });
}

void DBBDaemonGui::setPasswordClicked(bool showInfo)
{
    bool ok;
    QString text = QInputDialog::getText(this, tr("Set New Password"), tr("Password"), QLineEdit::Normal, "0000", &ok);
    if (ok && !text.isEmpty()) {
        std::string command = "{\"password\" : \"" + text.toStdString() + "\"}";

        if (executeCommandWrapper(command, (showInfo) ? DBB_PROCESS_INFOLAYER_STYLE_TOUCHBUTTON : DBB_PROCESS_INFOLAYER_STYLE_NO_INFO, [this](const std::string& cmdOut, dbb_cmd_execution_status_t status) {
                UniValue jsonOut;
                jsonOut.read(cmdOut);
                emit gotResponse(jsonOut, status, DBB_RESPONSE_TYPE_PASSWORD);
            })) {
            sessionPasswordDuringChangeProcess = sessionPassword;
            sessionPassword = text.toStdString();
        }
    }
}

void DBBDaemonGui::seedHardware()
{
    std::string command = "{\"seed\" : {\"source\" :\"create\","
                          "\"decrypt\": \"no\","
                          "\"salt\" : \"\"} }";

    executeCommandWrapper(command, (cachedWalletAvailableState) ? DBB_PROCESS_INFOLAYER_STYLE_TOUCHBUTTON : DBB_PROCESS_INFOLAYER_STYLE_NO_INFO, [this](const std::string& cmdOut, dbb_cmd_execution_status_t status) {
        UniValue jsonOut;
        jsonOut.read(cmdOut);
        emit gotResponse(jsonOut, status, DBB_RESPONSE_TYPE_CREATE_WALLET);
    });
}

/*
/////////////////
DBB Utils
/////////////////
*/
#pragma mark DBB Utils

void DBBDaemonGui::getRandomNumber()
{
    executeCommandWrapper("{\"random\" : \"true\" }", DBB_PROCESS_INFOLAYER_STYLE_NO_INFO, [this](const std::string& cmdOut, dbb_cmd_execution_status_t status) {
        UniValue jsonOut;
        jsonOut.read(cmdOut);
        emit gotResponse(jsonOut, status, DBB_RESPONSE_TYPE_RANDOM_NUM);
    });
}

void DBBDaemonGui::lockDevice()
{
    executeCommandWrapper("{\"device\" : \"lock\" }", DBB_PROCESS_INFOLAYER_STYLE_NO_INFO, [this](const std::string& cmdOut, dbb_cmd_execution_status_t status) {
        UniValue jsonOut;
        jsonOut.read(cmdOut);
        emit gotResponse(jsonOut, status, DBB_RESPONSE_TYPE_DEVICE_LOCK);
    });
}

/*
////////////////////////
Address Exporting  Stack
////////////////////////
*/
#pragma mark Get Address Stack

void DBBDaemonGui::showGetAddressDialog()
{
    getAddressDialog->show();
}

void DBBDaemonGui::getAddressGetXPub(const QString& keypath)
{
    getXPub(keypath.toStdString(), DBB_RESPONSE_TYPE_XPUB_GET_ADDRESS, DBB_ADDRESS_STYLE_P2PKH);
}

/*
/////////////////
DBB Backup Stack
/////////////////
*/
#pragma mark DBB Backup

void DBBDaemonGui::showBackupDialog()
{
    backupDialog->show();
    listBackup();
}

void DBBDaemonGui::addBackup()
{
    std::time_t rawtime;
    std::tm* timeinfo;
    char buffer[80];

    std::time(&rawtime);
    timeinfo = std::localtime(&rawtime);

    std::strftime(buffer, 80, "%Y-%m-%d-%H-%M-%S", timeinfo);
    std::string timeStr(buffer);

    std::string command = "{\"backup\" : {\"encrypt\" :\"no\","
                          "\"filename\": \"backup-" +
                          timeStr + ".bak\"} }";

    executeCommandWrapper(command, DBB_PROCESS_INFOLAYER_STYLE_NO_INFO, [this](const std::string& cmdOut, dbb_cmd_execution_status_t status) {
        UniValue jsonOut;
        jsonOut.read(cmdOut);
        emit gotResponse(jsonOut, status, DBB_RESPONSE_TYPE_ADD_BACKUP);
    });
}

void DBBDaemonGui::listBackup()
{
    std::string command = "{\"backup\" : \"list\" }";

    executeCommandWrapper(command, DBB_PROCESS_INFOLAYER_STYLE_NO_INFO, [this](const std::string& cmdOut, dbb_cmd_execution_status_t status) {
        UniValue jsonOut;
        jsonOut.read(cmdOut);
        emit gotResponse(jsonOut, status, DBB_RESPONSE_TYPE_LIST_BACKUP);
    });

    backupDialog->showLoading();
}

void DBBDaemonGui::eraseAllBackups()
{
    QMessageBox::StandardButton reply = QMessageBox::question(this, tr("Erase All Backups?"), tr("Are your sure you want to erase all backups"), QMessageBox::Yes | QMessageBox::No);
    if (reply == QMessageBox::No)
        return;

    std::string command = "{\"backup\" : \"erase\" }";

    executeCommandWrapper(command, DBB_PROCESS_INFOLAYER_STYLE_NO_INFO, [this](const std::string& cmdOut, dbb_cmd_execution_status_t status) {
        UniValue jsonOut;
        jsonOut.read(cmdOut);
        emit gotResponse(jsonOut, status, DBB_RESPONSE_TYPE_ERASE_BACKUP);
    });

    backupDialog->showLoading();
}

void DBBDaemonGui::restoreBackup(const QString& backupFilename)
{
    std::string command = "{\"seed\" : {\"source\" :\"" + backupFilename.toStdString() + "\","
                                                                                         "\"decrypt\": \"no\","
                                                                                         "\"salt\" : \"\"} }";

    executeCommandWrapper(command, DBB_PROCESS_INFOLAYER_STYLE_TOUCHBUTTON, [this](const std::string& cmdOut, dbb_cmd_execution_status_t status) {
        UniValue jsonOut;
        jsonOut.read(cmdOut);
        emit gotResponse(jsonOut, status, DBB_RESPONSE_TYPE_CREATE_WALLET);
    });

    backupDialog->close();
}

/*
 ///////////////////////////////////
 DBB USB Commands (Response Parsing)
 ///////////////////////////////////
*/
#pragma mark DBB USB Commands (Response Parsing)

void DBBDaemonGui::parseResponse(const UniValue& response, dbb_cmd_execution_status_t status, dbb_response_type_t tag, int subtag)
{
    processCommand = false;
    setLoading(false);

    if (response.isObject()) {
        UniValue errorObj = find_value(response, "error");
        UniValue touchbuttonObj = find_value(response, "touchbutton");
        bool touchErrorShowed = false;

        if (touchbuttonObj.isStr()) {
            showAlert(tr("Touchbutton"), QString::fromStdString(touchbuttonObj.get_str()));
            touchErrorShowed = true;
        }

        if (errorObj.isObject()) {
            //error found
            UniValue errorCodeObj = find_value(errorObj, "code");
            UniValue errorMessageObj = find_value(errorObj, "message");
            if (errorCodeObj.isNum() && errorCodeObj.get_int() == 108) {
                showAlert(tr("Password Error"), tr("Password Wrong. %1").arg(QString::fromStdString(errorMessageObj.get_str())));

                //try again
                askForSessionPassword();
            } else if (errorCodeObj.isNum() && errorCodeObj.get_int() == 110) {
                showAlert(tr("Password Error"), tr("Device Reset. %1").arg(QString::fromStdString(errorMessageObj.get_str())), true);
            } else if (errorCodeObj.isNum() && errorCodeObj.get_int() == 101) {
                showAlert(tr("Password Error"), QString::fromStdString(errorMessageObj.get_str()));

                sessionPassword.clear();
                setPasswordClicked(false);
            } else {
                //password wrong
                showAlert(tr("Error"), QString::fromStdString(errorMessageObj.get_str()));
            }
        } else if (tag == DBB_RESPONSE_TYPE_INFO) {
            UniValue deviceObj = find_value(response, "device");
            if (deviceObj.isObject()) {
                UniValue version = find_value(deviceObj, "version");
                UniValue name = find_value(deviceObj, "name");
                UniValue seeded = find_value(deviceObj, "seeded");
                UniValue lock = find_value(deviceObj, "lock");
                UniValue sdcard = find_value(deviceObj, "sdcard");
                bool walletAvailable = (seeded.isBool() && seeded.isTrue());
                bool lockAvailable = (lock.isBool() && lock.isTrue());

                if (version.isStr())
                    this->ui->versionLabel->setText(QString::fromStdString(version.get_str()));
                if (name.isStr())
                    this->ui->deviceNameLabel->setText(QString::fromStdString(name.get_str()));

                updateOverviewFlags(walletAvailable, lockAvailable, false);

                //enable UI
                passwordAccepted();
                deviceIsReadyToInteract();

                if (sdcard.isBool() && sdcard.isTrue() && walletAvailable && !sdcardWarned)
                {
                    showAlert(tr("Please remove your SDCard!"), "Don't keep the SDCard in your Digitalbitbox unless your are doing backups or restores");
                    sdcardWarned = true;
                }
            }
        } else if (tag == DBB_RESPONSE_TYPE_CREATE_WALLET) {
            UniValue touchbuttonObj = find_value(response, "touchbutton");
            UniValue seedObj = find_value(response, "seed");
            UniValue errorObj = find_value(response, "error");
            QString errorString;

            if (errorObj.isObject()) {
                UniValue errorMsgObj = find_value(errorObj, "message");
                if (errorMsgObj.isStr())
                    errorString = QString::fromStdString(errorMsgObj.get_str());
            }
            if (!touchbuttonObj.isNull() && touchbuttonObj.isObject()) {
                UniValue errorObj = find_value(touchbuttonObj, "error");
                if (!errorObj.isNull() && errorObj.isStr())
                    errorString = QString::fromStdString(errorObj.get_str());
            }
            if (!seedObj.isNull() && seedObj.isStr() && seedObj.get_str() == "success") {
                QMessageBox::information(this, tr("Wallet Created"), tr("Your wallet has been created successfully!"), QMessageBox::Ok);
                getInfo();
            } else {
                if (!touchErrorShowed)
                    showAlert(tr("Wallet Error"), errorString);
            }
        } else if (tag == DBB_RESPONSE_TYPE_PASSWORD) {
            UniValue passwordObj = find_value(response, "password");
            if (status != DBB_CMD_EXECUTION_STATUS_OK || (passwordObj.isStr() && passwordObj.get_str() == "success")) {
                sessionPasswordDuringChangeProcess.clear();

                //could not decrypt, password was changed successfully
                QMessageBox::information(this, tr("Password Set"), tr("Password has been set successfully!"), QMessageBox::Ok);
                getInfo();
            } else {
                QString errorString;
                UniValue touchbuttonObj = find_value(response, "touchbutton");
                if (!touchbuttonObj.isNull() && touchbuttonObj.isObject()) {
                    UniValue errorObj = find_value(touchbuttonObj, "error");
                    if (!errorObj.isNull() && errorObj.isStr())
                        errorString = QString::fromStdString(errorObj.get_str());
                }

                //reset password in case of an error
                sessionPassword = sessionPasswordDuringChangeProcess;
                sessionPasswordDuringChangeProcess.clear();

                showAlert(tr("Password Error"), tr("Could not set password (error: %1)!").arg(errorString));
            }
        } else if (tag == DBB_RESPONSE_TYPE_XPUB_MS_MASTER) {
            UniValue xPubKeyUV = find_value(response, "xpub");
            QString errorString;

            if (!xPubKeyUV.isNull() && xPubKeyUV.isStr()) {
                btc_hdnode node;
                bool r = btc_hdnode_deserialize(xPubKeyUV.get_str().c_str(), &btc_chain_main, &node);

                char outbuf[112];
                btc_hdnode_serialize_public(&node, &btc_chain_test, outbuf, sizeof(outbuf));

                std::string xPubKeyNew(outbuf);

                //0 = singlewallet
                if (subtag == 0)
                    singleWallet->client.setMasterPubKey(xPubKeyNew);
                else
                    vMultisigWallets[0]->client.setMasterPubKey(xPubKeyNew);

                emit XPubForCopayWalletIsAvailable(subtag);
            } else {
                if (xPubKeyUV.isObject()) {
                    UniValue errorObj = find_value(xPubKeyUV, "error");
                    if (!errorObj.isNull() && errorObj.isStr())
                        errorString = QString::fromStdString(errorObj.get_str());
                }

                showAlert(tr("Join Wallet Error"), tr("Error joining Copay Wallet (%1)").arg(errorString));
            }
        } else if (tag == DBB_RESPONSE_TYPE_XPUB_MS_REQUEST) {
            UniValue requestXPubKeyUV = find_value(response, "xpub");
            QString errorString;

            if (!requestXPubKeyUV.isNull() && requestXPubKeyUV.isStr()) {
                btc_hdnode node;
                bool r = btc_hdnode_deserialize(requestXPubKeyUV.get_str().c_str(), &btc_chain_main, &node);

                char outbuf[112];
                btc_hdnode_serialize_public(&node, &btc_chain_test, outbuf, sizeof(outbuf));

                std::string xRequestKeyNew(outbuf);

                if (subtag == 0)
                    singleWallet->client.setRequestPubKey(xRequestKeyNew);
                else
                    vMultisigWallets[0]->client.setRequestPubKey(xRequestKeyNew);

                emit RequestXPubKeyForCopayWalletIsAvailable(subtag);
            } else {
                if (requestXPubKeyUV.isObject()) {
                    UniValue errorObj = find_value(requestXPubKeyUV, "error");
                    if (!errorObj.isNull() && errorObj.isStr())
                        errorString = QString::fromStdString(errorObj.get_str());
                }

                showAlert(tr("Join Wallet Error"), tr("Error joining Copay Wallet (%1)").arg(errorString));
            }
        } else if (tag == DBB_RESPONSE_TYPE_XPUB_VERIFY) {
            UniValue responseMutable = response;
            UniValue requestXPubKeyUV = find_value(response, "xpub");
            QString errorString;
            if (requestXPubKeyUV.isStr()) {
                //pass the response to the verification devices
                if (subtag == DBB_ADDRESS_STYLE_MULTISIG_1OF1)
                    responseMutable.pushKV("type", "p2sh_ms_1of1");
                if (subtag == DBB_ADDRESS_STYLE_P2PKH)
                    responseMutable.pushKV("type", "p2pkh");
                if (websocketServer->sendStringToAllClients(responseMutable.write()) == 0)
                    showAlert(tr("No device found"), tr("Please run the verification app on your smartphone and make sure you have paired your device"));
            }
        } else if (tag == DBB_RESPONSE_TYPE_ERASE) {
            UniValue resetObj = find_value(response, "reset");
            if (resetObj.isStr() && resetObj.get_str() == "success") {
                QMessageBox::information(this, tr("Erase"), tr("Device was erased successfully"), QMessageBox::Ok);
                sessionPasswordDuringChangeProcess.clear();

                resetInfos();
                getInfo();
            } else {
                //reset password in case of an error
                sessionPassword = sessionPasswordDuringChangeProcess;
                sessionPasswordDuringChangeProcess.clear();

                if (!touchErrorShowed)
                    showAlert(tr("Erase error"), tr("Could not reset device"));
            }
        } else if (tag == DBB_RESPONSE_TYPE_LIST_BACKUP && backupDialog) {
            UniValue backupObj = find_value(response, "backup");
            if (backupObj.isArray()) {
                std::vector<std::string> data;
                for (const UniValue& obj : backupObj.getValues())
                {
                    data.push_back(obj.get_str());
                }
                backupDialog->showList(data);
            }
        } else if (tag == DBB_RESPONSE_TYPE_ADD_BACKUP && backupDialog) {
            listBackup();
        } else if (tag == DBB_RESPONSE_TYPE_ERASE_BACKUP && backupDialog) {
            listBackup();
        } else if (tag == DBB_RESPONSE_TYPE_RANDOM_NUM) {
            UniValue randomNumObj = find_value(response, "random");
            if (randomNumObj.isStr()) {
                QMessageBox::information(this, tr("Random Number"), QString::fromStdString(randomNumObj.get_str()), QMessageBox::Ok);
            }
        } else if (tag == DBB_RESPONSE_TYPE_DEVICE_LOCK) {
            bool suc = false;

            //check device:lock response and give appropriate user response
            UniValue deviceObj = find_value(response, "device");
            if (deviceObj.isObject()) {
                UniValue lockObj = find_value(deviceObj, "lock");
                if (lockObj.isBool() && lockObj.isTrue())
                    suc = true;
            }
            if (suc)
                QMessageBox::information(this, tr("Success"), tr("Your device is now locked"), QMessageBox::Ok);
            else
                showAlert(tr("Error"), tr("Could not lock your device"));

            //reload device infos
            resetInfos();
            getInfo();
        } else {
        }

        //no else if because we want to hide the blocker view in case of an error
        if (tag == DBB_RESPONSE_TYPE_VERIFYPASS_ECDH)
        {
            if (errorObj.isObject()) {
                showAlert(tr("Error"), tr("Verification Device Pairing Failed"));
            }
            hideModalInfo();
            websocketServer->sendDataToClientInECDHParingState(response);
        }

        if (tag == DBB_RESPONSE_TYPE_XPUB_GET_ADDRESS) {
            getAddressDialog->updateAddress(response);
        }
    }
}

/*
/////////////////
copay single- and multisig wallet stack
/////////////////
*/
#pragma mark copay single- and multisig wallet stack

void DBBDaemonGui::createSingleWallet()
{
    if (!singleWallet->client.IsSeeded()) {
        getXPubKeyForCopay(0);
    } else {
        SingleWalletUpdateWallets();
    }
}

void DBBDaemonGui::getNewAddress()
{
    if (singleWallet->client.IsSeeded()) {
        DBBNetThread* thread = DBBNetThread::DetachThread();
        thread->currentThread = std::thread([this]() {
            std::string walletsResponse;

            std::string address;
            std::string keypath;
            singleWallet->client.GetNewAddress(address, keypath);
            singleWallet->rewriteKeypath(keypath);
            emit walletAddressIsAvailable(this->singleWallet, address, keypath);
        });

        setNetLoading(true);
    }
}

void DBBDaemonGui::verifyAddress()
{
    getXPub(ui->keypathLabel->text().toStdString(), DBB_RESPONSE_TYPE_XPUB_VERIFY, DBB_ADDRESS_STYLE_P2PKH);
}
void DBBDaemonGui::getXPub(const std::string& keypath,  dbb_response_type_t response_type, dbb_address_style_t address_type)
{
    executeCommandWrapper("{\"xpub\":\"" + keypath + "\"}", DBB_PROCESS_INFOLAYER_STYLE_NO_INFO, [this,response_type,address_type](const std::string& cmdOut, dbb_cmd_execution_status_t status) {
        UniValue jsonOut;
        jsonOut.read(cmdOut);
        emit gotResponse(jsonOut, status, response_type, address_type);
    });
}

void DBBDaemonGui::updateReceivingAddress(DBBWallet *wallet, const std::string &newAddress, const std::string &info)
{
    setNetLoading(false);
    this->ui->currentAddress->setText(QString::fromStdString(newAddress));

    if (newAddress.size() <= 0)
        return;

    std::string uri = "bitcoin://"+newAddress;

    QRcode *code = QRcode_encodeString(uri.c_str(), 0, QR_ECLEVEL_L, QR_MODE_8, 1);
    if (code)
    {

        QImage myImage = QImage(code->width + 8, code->width + 8, QImage::Format_RGB32);
        myImage.fill(0xffffff);
        unsigned char *p = code->data;
        for (int y = 0; y < code->width; y++)
        {
            for (int x = 0; x < code->width; x++)
            {
                myImage.setPixel(x + 4, y + 4, ((*p & 1) ? 0x0 : 0xffffff));
                p++;
            }
        }
        QRcode_free(code);

        QIcon qrCode;
        QPixmap pixMap = QPixmap::fromImage(myImage).scaled(240, 240);
        qrCode.addPixmap(pixMap, QIcon::Normal);
        qrCode.addPixmap(pixMap, QIcon::Disabled);
        ui->qrCode->setIcon(qrCode);
    }

    ui->qrCode->setToolTip(QString::fromStdString(info));
    ui->keypathLabel->setText(QString::fromStdString(info));
}

void DBBDaemonGui::createTxProposalPressed()
{
    UniValue proposalOut;
    std::string errorOut;

    int64_t amount = 0;
    if (!DBB::ParseMoney(this->ui->sendAmount->text().toStdString(), amount))
        return showAlert("Error", "Invalid amount");

    if (!singleWallet->client.CreatePaymentProposal(this->ui->sendToAddress->text().toStdString(), amount, 2000, proposalOut, errorOut)) {
        showAlert("Error", QString::fromStdString(errorOut));
    } else {
        PaymentProposalAction(singleWallet, proposalOut, ProposalActionTypeAccept);
    }

    SingleWalletUpdateWallets();
}

void DBBDaemonGui::reportPaymentProposalPost(DBBWallet* wallet, const UniValue& proposal)
{
    QMessageBox::information(this, tr("Success"), tr("Transaction was sent successfully"), QMessageBox::Ok);
}

void DBBDaemonGui::joinCopayWalletClicked()
{
    bool isSeeded = vMultisigWallets[0]->client.IsSeeded();

    if (!isSeeded) {
        //if there is no xpub and request key, seed over DBB
        getXPubKeyForCopay(1);
    } else {
        //send a join request
        joinMultisigWalletInitiate(vMultisigWallets[0]);
    }
}

void DBBDaemonGui::joinMultisigWalletInitiate(DBBWallet* wallet)
{
    bool ok;
    QString text = QInputDialog::getText(this, tr("Join Copay Wallet"), tr("Wallet Invitation Code"), QLineEdit::Normal, "", &ok);
    if (!ok || text.isEmpty())
        return;

    // parse invitation code
    std::unique_lock<std::mutex> lock(this->cs_vMultisigWallets);

    BitpayWalletInvitation invitation;
    if (!wallet->client.ParseWalletInvitation(text.toStdString(), invitation)) {
        showAlert(tr("Invalid Invitation"), tr("Your Copay Wallet Invitation is invalid"));
        return;
    }

    std::string result;
    bool ret = wallet->client.JoinWallet(wallet->participationName, invitation, result);

    if (!ret) {
        UniValue responseJSON;
        std::string additionalErrorText = "unknown";
        if (responseJSON.read(result)) {
            UniValue errorText;
            errorText = find_value(responseJSON, "message");
            if (!errorText.isNull() && errorText.isStr())
                additionalErrorText = errorText.get_str();
        }

        showAlert(tr("Copay Wallet Response"), tr("Joining the wallet failed (%1)").arg(QString::fromStdString(additionalErrorText)));
    } else {
        QMessageBox::information(this, tr("Copay Wallet Response"), tr("Successfull joined Copay Wallet"), QMessageBox::Ok);
    }
}

void DBBDaemonGui::getXPubKeyForCopay(int walletIndex)
{
    DBBWallet* wallet = vMultisigWallets[0];
    if (walletIndex == 0)
        wallet = singleWallet;

    std::string baseKeyPath;
    {
        std::unique_lock<std::mutex> lock(this->cs_vMultisigWallets);
        baseKeyPath = wallet->baseKeyPath;
    }

    executeCommandWrapper("{\"xpub\":\"" + baseKeyPath + "/45'\"}", DBB_PROCESS_INFOLAYER_STYLE_NO_INFO, [this, walletIndex](const std::string& cmdOut, dbb_cmd_execution_status_t status) {
        UniValue jsonOut;
        jsonOut.read(cmdOut);
        emit gotResponse(jsonOut, status, DBB_RESPONSE_TYPE_XPUB_MS_MASTER, walletIndex);
    });
}

void DBBDaemonGui::getRequestXPubKeyForCopay(int walletIndex)
{
    DBBWallet* wallet = vMultisigWallets[0];
    if (walletIndex == 0)
        wallet = singleWallet;

    std::string baseKeyPath = wallet->baseKeyPath;

    //try to get the xpub for seeding the request private key (ugly workaround)
    //we cannot export private keys from a hardware wallet
    executeCommandWrapper("{\"xpub\":\"" + baseKeyPath + "/1'/0\"}", DBB_PROCESS_INFOLAYER_STYLE_NO_INFO, [this, walletIndex](const std::string& cmdOut, dbb_cmd_execution_status_t status) {
        UniValue jsonOut;
        jsonOut.read(cmdOut);
        emit gotResponse(jsonOut, status, DBB_RESPONSE_TYPE_XPUB_MS_REQUEST, walletIndex);
    });
}

void DBBDaemonGui::joinCopayWalletWithXPubKey(int walletIndex)
{
    DBBWallet* wallet = vMultisigWallets[0];
    if (walletIndex == 0)
        wallet = singleWallet;

    if (walletIndex == 0) {
        //single wallet, create wallet first
        wallet->client.CreateWallet(wallet->participationName);
    } else {
        //set the keys and try to join the wallet
        joinMultisigWalletInitiate(wallet);
    }
}

void DBBDaemonGui::hidePaymentProposalsWidget()
{
    if (currentPaymentProposalWidget) {
        currentPaymentProposalWidget->hide();
        delete currentPaymentProposalWidget;
        currentPaymentProposalWidget = NULL;
    }

    this->ui->noProposalsAvailable->setVisible(true);
}

void DBBDaemonGui::updateWallet(DBBWallet* wallet)
{
    if (wallet == singleWallet) {
        SingleWalletUpdateWallets();
    } else
        MultisigUpdateWallets();
}

void DBBDaemonGui::MultisigUpdateWallets()
{
    DBBWallet* wallet = vMultisigWallets[0];
    if (!wallet->client.IsSeeded())
        return;

    multisigWalletIsUpdating = true;
    executeNetUpdateWallet(wallet, [wallet, this](bool walletsAvailable, const std::string& walletsResponse) {
        emit getWalletsResponseAvailable(wallet, walletsAvailable, walletsResponse);
    });
}

void DBBDaemonGui::SingleWalletUpdateWallets()
{
    if (!singleWallet->client.IsSeeded())
    {
        ui->createSingleWallet->setText("Create Wallet");
        return;
    }
    ui->createSingleWallet->setText("Refresh");

    singleWalletIsUpdating = true;
    executeNetUpdateWallet(singleWallet, [this](bool walletsAvailable, const std::string& walletsResponse) {
        emit getWalletsResponseAvailable(this->singleWallet, walletsAvailable, walletsResponse);
    });

    DBBNetThread* thread = DBBNetThread::DetachThread();
    thread->currentThread = std::thread([this]() {
        std::string txHistoryResponse;
        bool transactionHistoryAvailable = this->singleWallet->client.GetTransactionHistory(txHistoryResponse);


        UniValue data;
        if (transactionHistoryAvailable)
            data.read(txHistoryResponse);

        emit getTransactionHistoryAvailable(this->singleWallet, transactionHistoryAvailable, data);
    });
}

void DBBDaemonGui::updateUIMultisigWallets(const UniValue& walletResponse)
{
    vMultisigWallets[0]->updateData(walletResponse);

    if (vMultisigWallets[0]->currentPaymentProposals.isArray()) {
        this->ui->proposalsLabel->setText(tr("Current Payment Proposals (%1)").arg(vMultisigWallets[0]->currentPaymentProposals.size()));
    }

    //TODO, add a monetary amount / unit helper function
    this->ui->multisigBalance->setText(tr("%1 Bits").arg(vMultisigWallets[0]->totalBalance));
    this->ui->multisigWalletName->setText(QString::fromStdString(vMultisigWallets[0]->walletRemoteName));
}

void DBBDaemonGui::updateUISingleWallet(const UniValue& walletResponse)
{
    singleWallet->updateData(walletResponse);

    //TODO, add a monetary amount / unit helper function

    QString balance = QString::fromStdString(DBB::formatMoney(singleWallet->totalBalance));

    this->ui->balanceLabel->setText(tr("%1 BTC").arg(balance));
    this->ui->singleWalletBalance->setText(tr("%1 BTC").arg(balance));
}

void DBBDaemonGui::updateTransactionTable(DBBWallet *wallet, bool historyAvailable, const UniValue &history)
{
    ui->tableWidget->setModel(NULL);

    if (!historyAvailable || !history.isArray())
        return;


    transactionTableModel = new  QStandardItemModel(history.size(),3,this);

    transactionTableModel->setHeaderData( 0, Qt::Horizontal, QObject::tr("Type") );
    transactionTableModel->setHeaderData( 0, Qt::Horizontal, QObject::tr("Amount") );
    transactionTableModel->setHeaderData( 0, Qt::Horizontal, QObject::tr("Fees") );
    transactionTableModel->setHeaderData( 0, Qt::Horizontal, QObject::tr("Date") );

    int cnt = 0;
    for (const UniValue &obj : history.getValues())
    {
        UniValue timeUV = find_value(obj, "time");
        if (timeUV.isNum())
        {
            QDateTime timestamp;
            timestamp.setTime_t(timeUV.get_int64());
            QStandardItem *item = new QStandardItem(timestamp.toString(Qt::SystemLocaleShortDate));
            transactionTableModel->setItem(cnt, 0, item);
        }

        UniValue actionUV = find_value(obj, "action");
        if (actionUV.isStr())
        {
            QString iconName = ":/icons/tx_" + QString::fromStdString(actionUV.get_str());
            QStandardItem *item = new QStandardItem(QIcon(iconName), QString::fromStdString(actionUV.get_str()) );
            transactionTableModel->setItem(cnt, 1, item);
        }

        UniValue amountUV = find_value(obj, "amount");
        if (amountUV.isNum())
        {
            QStandardItem *item = new QStandardItem(QString::fromStdString(DBB::formatMoney(amountUV.get_int64())) + " BTC" );
            transactionTableModel->setItem(cnt, 2, item);
        }

        UniValue feeUV = find_value(obj, "fees");
        if (feeUV.isNum())
        {
            QStandardItem *item = new QStandardItem(QString::number(feeUV.get_int64()) + " Satoshis" );
            transactionTableModel->setItem(cnt, 3, item);
        }

        cnt++;
    }

    ui->tableWidget->setModel(transactionTableModel);
}

void DBBDaemonGui::executeNetUpdateWallet(DBBWallet* wallet, std::function<void(bool, std::string&)> cmdFinished)
{
    DBBNetThread* thread = DBBNetThread::DetachThread();
    thread->currentThread = std::thread([wallet, cmdFinished]() {
        std::string walletsResponse;

        //std::unique_lock<std::mutex> lock(this->cs_vMultisigWallets);
        bool walletsAvailable = wallet->client.GetWallets(walletsResponse);
        cmdFinished(walletsAvailable, walletsResponse);
    });

    setNetLoading(true);
}

void DBBDaemonGui::parseWalletsResponse(DBBWallet* wallet, bool walletsAvailable, const std::string& walletsResponse)
{
    setNetLoading(false);

    if (wallet == singleWallet)
        singleWalletIsUpdating = false;
    else
        multisigWalletIsUpdating = false;

    UniValue response;
    if (response.read(walletsResponse)) {
        if (response.isObject()) {
            printf("Wallet: %s\n", response.write(true, 2).c_str());

            if (wallet == singleWallet)
                updateUISingleWallet(response);
            else {
                updateUIMultisigWallets(response);
                MultisigUpdatePaymentProposals(response);
            }
        }
    }
}

bool DBBDaemonGui::MultisigUpdatePaymentProposals(const UniValue& response)
{
    bool ret = false;
    int copayerIndex = INT_MAX;

    UniValue pendingTxps;
    pendingTxps = find_value(response, "pendingTxps");
    if (!pendingTxps.isNull() && pendingTxps.isArray()) {
        std::unique_lock<std::mutex> lock(this->cs_vMultisigWallets);
        vMultisigWallets[0]->currentPaymentProposals = pendingTxps;

        printf("pending txps: %s", pendingTxps.write(2, 2).c_str());
        std::vector<UniValue> values = pendingTxps.getValues();
        if (values.size() == 0) {
            hidePaymentProposalsWidget();
            return false;
        }

        size_t cnt = 0;

        for (const UniValue& oneProposal : values) {
            QString amount;
            QString toAddress;

            UniValue toAddressUni = find_value(oneProposal, "toAddress");
            UniValue amountUni = find_value(oneProposal, "amount");
            UniValue actions = find_value(oneProposal, "actions");

            bool skipProposal = false;
            if (actions.isArray()) {
                for (const UniValue& oneAction : actions.getValues()) {
                    UniValue copayerId = find_value(oneAction, "copayerId");
                    UniValue actionType = find_value(oneAction, "type");

                    if (!copayerId.isStr() || !actionType.isStr())
                        continue;

                    if (vMultisigWallets[0]->client.GetCopayerId() == copayerId.get_str() && actionType.get_str() == "accept") {
                        skipProposal = true;
                        break;
                    }
                }
            }

            UniValue isUni = find_value(oneProposal, "id");
            if (isUni.isStr())
                MultisigShowPaymentProposal(pendingTxps, isUni.get_str());

            return true;

            QMessageBox::StandardButton reply = QMessageBox::question(this, tr("Payment Proposal Available"), tr("Do you want to sign: pay %1BTC to %2").arg(amount, toAddress), QMessageBox::Yes | QMessageBox::No);
            if (reply == QMessageBox::No)
                return false;
        } //end proposal loop
    }

    return ret;
}

bool DBBDaemonGui::MultisigShowPaymentProposal(const UniValue& pendingTxps, const std::string& targetID)
{
    if (pendingTxps.isArray()) {
        std::vector<UniValue> values = pendingTxps.getValues();
        if (values.size() == 0) {
            hidePaymentProposalsWidget();
            return false;
        }

        size_t cnt = 0;
        for (const UniValue& oneProposal : values) {
            UniValue idUni = find_value(oneProposal, "id");
            if (!idUni.isStr() || idUni.get_str() != targetID) {
                cnt++;
                continue;
            }

            std::string prevProposalID;
            std::string nextProposalID;

            if (cnt > 0) {
                UniValue idUni = find_value(values[cnt - 1], "id");
                if (idUni.isStr())
                    prevProposalID = idUni.get_str();
            }

            if (cnt < values.size() - 1) {
                UniValue idUni = find_value(values[cnt + 1], "id");
                if (idUni.isStr())
                    nextProposalID = idUni.get_str();
            }

            if (!currentPaymentProposalWidget) {
                currentPaymentProposalWidget = new PaymentProposal(this->ui->copay);
                connect(currentPaymentProposalWidget, SIGNAL(processProposal(DBBWallet*, const UniValue&, int)), this, SLOT(PaymentProposalAction(DBBWallet*, const UniValue&, int)));
                connect(currentPaymentProposalWidget, SIGNAL(shouldDisplayProposal(const UniValue&, const std::string&)), this, SLOT(MultisigShowPaymentProposal(const UniValue&, const std::string&)));
            }

            currentPaymentProposalWidget->move(15, 115);
            currentPaymentProposalWidget->show();
            currentPaymentProposalWidget->SetData(vMultisigWallets[0], vMultisigWallets[0]->client.GetCopayerId(), pendingTxps, oneProposal, prevProposalID, nextProposalID);

            this->ui->noProposalsAvailable->setVisible(false);

            cnt++;
        }
    }
    return true;
}

void DBBDaemonGui::PaymentProposalAction(DBBWallet* wallet, const UniValue& paymentProposal, int actionType)
{
    std::unique_lock<std::mutex> lock(this->cs_vMultisigWallets);

    if (actionType == ProposalActionTypeReject) {
        wallet->client.RejectTxProposal(paymentProposal);
        MultisigUpdateWallets();
        return;
    } else if (actionType == ProposalActionTypeDelete) {
        wallet->client.DeleteTxProposal(paymentProposal);
        MultisigUpdateWallets();
        return;
    }
    std::vector<std::pair<std::string, std::vector<unsigned char> > > inputHashesAndPaths;
    std::string serTx;
    UniValue changeAddressData;
    wallet->client.ParseTxProposal(paymentProposal, changeAddressData, serTx, inputHashesAndPaths);

    //build sign command
    std::string hashCmd;
    for (const std::pair<std::string, std::vector<unsigned char> >& hashAndPathPair : inputHashesAndPaths) {
        std::string hexHash = DBB::HexStr((unsigned char*)&hashAndPathPair.second[0], (unsigned char*)&hashAndPathPair.second[0] + 32);

        hashCmd += "{ \"hash\" : \"" + hexHash + "\", \"keypath\" : \"" + wallet->baseKeyPath + "/45'/" + hashAndPathPair.first + "\" }, ";
    }
    hashCmd.pop_back();
    hashCmd.pop_back(); // remove ", "

    //build checkpubkeys
    UniValue checkpubObj = UniValue(UniValue::VARR);
    if (changeAddressData.isObject())
    {
        UniValue keypath = find_value(changeAddressData, "path");
        UniValue publicKeys = find_value(changeAddressData, "publicKeys");
        if (publicKeys.isArray())
        {
            for (const UniValue& pkey : publicKeys.getValues())
            {
                if (pkey.isStr())
                {
                    UniValue obj = UniValue(UniValue::VOBJ);
                    obj.pushKV("pubkey", pkey.get_str());
                    if (keypath.isStr())
                        obj.pushKV("keypath", keypath.get_str());
                    checkpubObj.push_back(obj);
                }
            }
        }
    }

    std::string hexHash = DBB::HexStr(&inputHashesAndPaths[0].second[0], &inputHashesAndPaths[0].second[0] + 32);


    std::string command = "{\"sign\": { \"type\": \"meta\", \"meta\" : \""+serTx+"\", \"data\" : [ " + hashCmd + " ], \"checkpub\" : "+checkpubObj.write()+" } }";
    printf("Command: %s\n", command.c_str());

    bool ret = false;
    executeCommandWrapper(command, DBB_PROCESS_INFOLAYER_STYLE_NO_INFO, [wallet, &ret, actionType, paymentProposal, inputHashesAndPaths, this](const std::string& cmdOut, dbb_cmd_execution_status_t status) {
        //send a signal to the main thread
        processCommand = false;
        setLoading(false);

        printf("cmd back: %s\n", cmdOut.c_str());
        UniValue jsonOut(UniValue::VOBJ);
        jsonOut.read(cmdOut);

        UniValue echoStr = find_value(jsonOut, "echo");
        if (!echoStr.isNull() && echoStr.isStr()) {
            emit shouldVerifySigning(wallet, paymentProposal, actionType, echoStr.get_str());
        } else {
            UniValue errorObj = find_value(jsonOut, "error");
            if (errorObj.isObject()) {
                //error found
                UniValue errorCodeObj = find_value(errorObj, "code");
                UniValue errorMessageObj = find_value(errorObj, "message");

                emit shouldHideVerificationInfo();
            }
            else
            {
                UniValue signObject = find_value(jsonOut, "sign");
                if (signObject.isArray()) {
                    std::vector<UniValue> vSignatureObjects;
                    vSignatureObjects = signObject.getValues();
                    if (vSignatureObjects.size() > 0) {
                        std::vector<std::string> sigs;

                        for (const UniValue& oneSig : vSignatureObjects) {
                            UniValue sigObject = find_value(oneSig, "sig");
                            UniValue pubKey = find_value(oneSig, "pubkey");
                            if (!sigObject.isNull() && sigObject.isStr()) {
                                sigs.push_back(sigObject.get_str());
                                //client.BroadcastProposal(values[0]);
                            }
                        }

                        emit shouldHideVerificationInfo();
                        emit signedProposalAvailable(wallet, paymentProposal, sigs);
                        ret = true;
                    }
                }
            }
        }
    });
}

void DBBDaemonGui::postSignaturesForPaymentProposal(DBBWallet* wallet, const UniValue& proposal, const std::vector<std::string>& vSigs)
{
    DBBNetThread* thread = DBBNetThread::DetachThread();
    thread->currentThread = std::thread([this, thread, wallet, proposal, vSigs]() {
        //thread->currentThread = ;
        wallet->postSignaturesForPaymentProposal(proposal, vSigs);
        wallet->broadcastPaymentProposal(proposal);

        //sleep 3 seconds to get time for the wallet server to process the transaction and response with the correct balance
        std::this_thread::sleep_for(std::chrono::milliseconds(3000));

        emit shouldUpdateWallet(wallet);
        emit paymentProposalUpdated(wallet, proposal);

        thread->completed();
    });
}

#pragma mark - WebSocket Stack (ECDH)

void DBBDaemonGui::sendECDHPairingRequest(const std::string &pubkey)
{
    if (!deviceReadyToInteract)
        return;
    
    executeCommandWrapper("{\"verifypass\": {\"ecdh\" : \"" + pubkey + "\"}}", DBB_PROCESS_INFOLAYER_STYLE_NO_INFO, [this](const std::string& cmdOut, dbb_cmd_execution_status_t status) {
        UniValue jsonOut;
        jsonOut.read(cmdOut);
        emit gotResponse(jsonOut, status, DBB_RESPONSE_TYPE_VERIFYPASS_ECDH);
    });
    showModalInfo("Pairing Verification Device");
}