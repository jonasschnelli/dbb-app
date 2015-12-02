// Copyright (c) 2015 Jonas Schnelli / Shift Devices AG
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "verificationdialog.h"
#include "ui/ui_verificationdialog.h"

VerificationDialog::VerificationDialog(QWidget *parent) :
QDialog(parent),
ui(new Ui::VerificationDialog)
{
    ui->setupUi(this);

    connect(ui->closeButton, SIGNAL(clicked()), this, SLOT(close()));
}


void VerificationDialog::setData(const QString& title, const QString& detailText, const std::string& qrCodeData)
{
    ui->titleLabel->setText(title);
    ui->detailTextLabel->setText(detailText);
    ui->qrCodeSequence->setData(qrCodeData);
}


VerificationDialog::~VerificationDialog()
{
    delete ui;
}
