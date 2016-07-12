// Copyright (c) 2015 Jonas Schnelli
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef DBB_MODALVIEW_H
#define DBB_MODALVIEW_H

#include <QKeyEvent>
#include <QWidget>

#include "ui/ui_modalview.h"

#include <univalue.h>

class ModalView : public QWidget
{
    Q_OBJECT

public:
    explicit ModalView(QWidget* parent = 0);
    ~ModalView();
    Ui::ModalView *ui;

signals:
    void newPasswordAvailable(const QString&, const QString&);
    void newDeviceNamePasswordAvailable(const QString&, const QString&);
    void newDeviceNameAvailable(const QString&);
    void signingShouldProceed(const QString&, void *, const UniValue&, int);
    void modalViewWillShowHide(bool);
    void shouldUpgradeFirmware();

public slots:
    void showOrHide(bool state = false);
    void showSetNewWallet();
    void showSetPassword();
    void showSetDeviceNameCreate();
    void showModalInfo(const QString &info, int helpType);
    void showTransactionVerification(bool twoFAlocked, bool showQRSqeuence = false);
    void deviceSubmitProvided();
    void deviceCancelProvided();
    void cleanse();
    void setDeviceHideAll();
    void setText(const QString& text);
    void updateIcon(const QIcon& icon);

    //we directly store the required transaction data in the modal view together with what we display to the user
    void setTXVerificationData(void *info, const UniValue& data, const std::string& echo, int type);

    void clearTXData();
    void detailButtonAction();
    void okButtonAction();
    void proceedFrom2FAToSigning(const QString &twoFACode);
    void twoFACancelPressed();

    void inputCheck(const QString& sham);
    void continuePressed();
    void upgradeFirmware();

protected:
    virtual void keyPressEvent(QKeyEvent *event);
    void setQrCodeVisibility(bool state);

private:
    bool visible;
    UniValue txData;
    std::string txEcho;
    int txType;
    void *txPointer;
};


#endif // DBB_MODALVIEW_H
