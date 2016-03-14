// Copyright (c) 2015 Jonas Schnelli / Shift Devices AG
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "modalview.h"

#include <QMessageBox>

#include "dbb_gui.h"
#include "dbb_util.h"

ModalView::ModalView(QWidget* parent) : QWidget(parent), ui(new Ui::ModalView), txPointer(0)
{
    ui->setupUi(this);

    connect(this->ui->setPassword0, SIGNAL(returnPressed()), this->ui->setPassword1, SLOT(setFocus()));
    connect(this->ui->setPassword0, SIGNAL(textChanged(const QString&)), this, SLOT(passwordCheck(const QString&)));
    connect(this->ui->setPassword1, SIGNAL(textChanged(const QString&)), this, SLOT(passwordCheck(const QString&)));
    connect(this->ui->setPassword1, SIGNAL(returnPressed()), this->ui->setPassword, SIGNAL(clicked()));
    connect(this->ui->setPassword, SIGNAL(clicked()), this, SLOT(setPasswordProvided()));
    connect(this->ui->cancelSetPassword, SIGNAL(clicked()), this, SLOT(cancelSetPasswordProvided()));

    connect(this->ui->okButton, SIGNAL(clicked()), this, SLOT(okButtonAction()));
    connect(this->ui->showDetailsButton, SIGNAL(clicked()), this, SLOT(detailButtonAction()));
    connect(this->ui->twoFACode, SIGNAL(returnPressed()), this, SLOT(twoFACodeReturnPressed()));
    connect(this->ui->abortButton, SIGNAL(clicked()), this, SLOT(twoFACanclePressed()));
    connect(this->ui->continueButton, SIGNAL(clicked()), this, SLOT(continuePressed()));

    ui->qrCodeSequence->useOnDarkBackground(true);
    visible = false;
}

ModalView::~ModalView()
{
    delete ui;
}

void ModalView::setText(const QString& text)
{
    ui->modalInfoLabel->setVisible(true);
    ui->modalInfoLabelLA->setVisible(true);
    ui->modalInfoLabel->setText(text);
}

void ModalView::cleanse()
{
    ui->setPassword0->clear();
    ui->setPassword1->clear();
}

void ModalView::setPasswordProvided()
{
    if (ui->setPassword0->text() != ui->setPassword1->text())
    {
        //: translation: password not identical text
        // showAlert(tr("Error"), tr("Passwords not identical"));
        //TODO
        return;
    }

    emit newPasswordAvailable(ui->setPassword0->text(), !ui->passwordInfo->isVisible());

    ui->setPassword0->setText("");
    ui->setPassword1->setText("");
}

void ModalView::cancelSetPasswordProvided()
{
    showOrHide();
}

void ModalView::showOrHide(bool state)
{
    if (state)
        setGeometry(-this->width(), 0, this->width(), this->height());

    QPropertyAnimation* animation = new QPropertyAnimation(this, "pos");
    animation->setDuration(300);
    animation->setStartValue(this->pos());
    animation->setEndValue(QPoint((state ? 0 : -this->width()), 0));
    animation->setEasingCurve(QEasingCurve::OutQuad);
    // to slide in call
    animation->start(QAbstractAnimation::DeleteWhenStopped);

    visible = state;
}

void ModalView::showSetPasswordInfo(bool showCleanInfo)
{
    ui->twoFACode->setVisible(false);
    ui->abortButton->setVisible(false);
    ui->continueButton->setVisible(false);
    ui->qrCodeSequence->setVisible(false);
    ui->showDetailsButton->setVisible(false);
    ui->okButton->setVisible(false);
    ui->setPasswordWidget->setVisible(true);
    ui->passwordInfo->setVisible(showCleanInfo);
    ui->cancelSetPassword->setVisible(!showCleanInfo);
    ui->modalInfoLabel->setVisible(false);
    ui->modalInfoLabelLA->setVisible(false);
    ui->modalInfoLabel->setText("");
    ui->modalInfoLabelLA->setText("");
    ui->setPassword0->setFocus();
    ui->modalIcon->setIcon(QIcon());

    ui->setPassword->setEnabled(false);
    showOrHide(true);
    ui->setPassword->setEnabled(false);
}

void ModalView::showModalInfo(const QString &info, int helpType)
{
    showOrHide(true);

    ui->setPasswordWidget->setVisible(false);
    ui->okButton->setVisible(false);
    ui->twoFACode->setVisible(false);
    ui->abortButton->setVisible(false);
    ui->qrCodeSequence->setVisible(false);
    ui->continueButton->setVisible(false);
    ui->showDetailsButton->setVisible(false);

    ui->modalInfoLabel->setVisible(true);
    ui->modalInfoLabelLA->setVisible(true);

    ui->modalInfoLabelLA->setText("");
    ui->modalInfoLabel->setText(info);
    QWidget* slide = this;
    // setup slide
    slide->setGeometry(-slide->width(), 0, slide->width(), slide->height());

    if (helpType == DBB_PROCESS_INFOLAYER_STYLE_TOUCHBUTTON)
    {
        QIcon newIcon;
        newIcon.addPixmap(QPixmap(":/icons/touchhelp"), QIcon::Normal);
        newIcon.addPixmap(QPixmap(":/icons/touchhelp"), QIcon::Disabled);
        ui->modalIcon->setIcon(newIcon);

        if (info.isNull() || info.size() == 0)
            ui->modalInfoLabel->setText(tr(""));
    }
    else if (helpType == DBB_PROCESS_INFOLAYER_STYLE_REPLUG)
    {
        QIcon newIcon;
        newIcon.addPixmap(QPixmap(":/icons/touchhelp_replug"), QIcon::Normal);
        newIcon.addPixmap(QPixmap(":/icons/touchhelp_replug"), QIcon::Disabled);
        ui->modalIcon->setIcon(newIcon);

        if (info.isNull() || info.size() == 0)
            ui->modalInfoLabel->setText(tr(""));
    }
    else if (helpType == DBB_PROCESS_INFOLAYER_CONFIRM_WITH_BUTTON || helpType == DBB_PROCESS_INFOLAYER_CONFIRM_WITH_BUTTON_WARNING)
    {
        if (helpType == DBB_PROCESS_INFOLAYER_CONFIRM_WITH_BUTTON_WARNING)
        {
            QIcon newIcon;
            newIcon.addPixmap(QPixmap(":/icons/modal_warning"), QIcon::Normal);
            newIcon.addPixmap(QPixmap(":/icons/modal_warning"), QIcon::Disabled);
            ui->modalIcon->setIcon(newIcon);
        }
        else
            ui->modalIcon->setIcon(QIcon());

        ui->okButton->setVisible(true);
        ui->okButton->setFocus();
    }
    else
    {
        ui->modalIcon->setIcon(QIcon());
    }


    // then a animation:
    QPropertyAnimation* animation = new QPropertyAnimation(slide, "pos");
    animation->setDuration(300);
    animation->setStartValue(slide->pos());
    animation->setEndValue(QPoint(0, 0));
    animation->setEasingCurve(QEasingCurve::OutQuad);
    // to slide in call
    animation->start(QAbstractAnimation::DeleteWhenStopped);
}

void ModalView::showTransactionVerification(bool twoFAlocked, bool showQRSqeuence)
{
    QString longString;

    longString += "Sending:<br />";

    UniValue amountUni = find_value(txData, "amount");
    if (amountUni.isNum())
    {
        longString += "<strong>"+QString::fromStdString(DBB::formatMoney(amountUni.get_int64()))+"</strong><br />";
    }

    UniValue toAddressUni = find_value(txData, "toAddress");
    if (toAddressUni.isStr())
    {
        longString += "to <strong>"+QString::fromStdString(toAddressUni.get_str())+"</strong><br />";
    }

    UniValue feeUni = find_value(txData, "fee");
    if (feeUni.isNum())
    {
        longString += "Fee: " + QString::fromStdString(DBB::formatMoney(feeUni.get_int64()));
    }

    showModalInfo("", DBB_PROCESS_INFOLAYER_STYLE_TOUCHBUTTON);
    ui->modalInfoLabelLA->setText(longString);
    ui->twoFACode->setVisible(twoFAlocked);
    ui->abortButton->setVisible(twoFAlocked);
    ui->qrCodeSequence->setData(txEcho);
    ui->modalInfoLabel->setVisible(true);
    ui->modalInfoLabelLA->setVisible(true);
    ui->showDetailsButton->setVisible(true);
    ui->showDetailsButton->setText(tr("Show Verification QR Codes"));

    if (showQRSqeuence)
    {
        ui->continueButton->setVisible(true);
        setQrCodeVisibility(true);
        updateIcon(QIcon());
        ui->showDetailsButton->setVisible(false);
    }

    if (twoFAlocked)
    {
        ui->continueButton->setVisible(true);
        ui->twoFACode->setFocus();
    }
}

void ModalView::detailButtonAction()
{
    setQrCodeVisibility(!ui->qrCodeSequence->isVisible());
}

void ModalView::setQrCodeVisibility(bool state)
{
    if (!state)
    {
        ui->showDetailsButton->setText(tr("Show Verification QR Codes"));
        ui->qrCodeSequence->setVisible(false);

        QIcon newIcon;
        newIcon.addPixmap(QPixmap(":/icons/touchhelp"), QIcon::Normal);
        newIcon.addPixmap(QPixmap(":/icons/touchhelp"), QIcon::Disabled);
        ui->modalIcon->setIcon(newIcon);
    }
    else
    {
        ui->showDetailsButton->setText(tr("Hide Verification Code"));
        ui->qrCodeSequence->setVisible(true);
        ui->modalIcon->setIcon(QIcon());
    }
}

void ModalView::twoFACodeReturnPressed()
{
    if (ui->twoFACode->text().size() == 0)
    {
        QMessageBox::warning(this, tr(""), tr("Enter the 2FA Code first"), QMessageBox::Ok);
        return;
    }

    if (txPointer)
    {
        ui->qrCodeSequence->setVisible(false);
        emit signingShouldProceed(ui->twoFACode->text(), txPointer, txData, txType);
        ui->twoFACode->setText("");
        ui->twoFACode->setVisible(false);
        ui->abortButton->setVisible(false);
    }
}

void ModalView::twoFACanclePressed()
{
    ui->qrCodeSequence->setVisible(false);
    ui->twoFACode->setText("");
    ui->twoFACode->setVisible(false);
    ui->abortButton->setVisible(false);

    if (txPointer)
    {
        emit signingShouldProceed(QString(), NULL, txData, txType);
    }
}

void ModalView::okButtonAction()
{
    showOrHide();
}

void ModalView::setTXVerificationData(void *info, const UniValue& data, const std::string& echo, int type)
{
    txPointer = info;
    txData = data;
    txEcho = echo;
    txType = type;
}

void ModalView::clearTXData()
{
    txPointer = NULL;
    txData = UniValue(UniValue::VNULL);
    txEcho.clear();
    txType = 0;
}

void ModalView::updateIcon(const QIcon& icon)
{
    ui->modalIcon->setIcon(icon);
}

void ModalView::keyPressEvent(QKeyEvent* event){
    if ((event->key()==Qt::Key_Return) && visible && ui->okButton->isVisible())
        showOrHide(false);

}

void ModalView::passwordCheck(const QString& password0){
    if (ui->setPassword0->text().size() < 4)
    {
        ui->passwordWarning->setText(tr("Password too short"));
        ui->setPassword->setEnabled(false);
        return;
    }
    if (ui->setPassword0->text() != ui->setPassword1->text())
    {
        ui->setPassword->setEnabled(false);
        ui->passwordWarning->setText(tr("Password not identical"));
        return;
    }
    ui->setPassword->setEnabled(true);
    ui->passwordWarning->setText("");
}

void ModalView::continuePressed()
{
    if (ui->twoFACode->isVisible())
    {
        twoFACodeReturnPressed();
        return;
    }

    if (txPointer)
    {
        setQrCodeVisibility(false);
        ui->continueButton->setVisible(false);
        emit signingShouldProceed(ui->twoFACode->text(), txPointer, txData, txType);
    }
}