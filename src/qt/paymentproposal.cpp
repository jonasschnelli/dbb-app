// Copyright (c) 2015 Jonas Schnelli / Shift Devices AG
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "paymentproposal.h"
#include "ui/ui_paymentproposal.h"

#include <QStringListModel>
#include <QStringList>

PaymentProposal::PaymentProposal(QWidget *parent) :
QWidget(parent),
ui(new Ui::PaymentProposal)
{
    ui->setupUi(this);

    ui->bgWidget->setStyleSheet("background-color: rgba(0,0,0,15);");
    ui->amountLabelKey->setStyleSheet("font-weight: bold;");
    ui->feeLabelKey->setStyleSheet("font-weight: bold;");

    connect(this->ui->acceptButton, SIGNAL(clicked()), this, SLOT(acceptPressed()));
    connect(this->ui->rejectButton, SIGNAL(clicked()), this, SLOT(rejectPressed()));
}


void PaymentProposal::SetData(const UniValue &proposalDataIn)
{
    proposalData = proposalDataIn;
    
    UniValue toAddressUni = find_value(proposalData, "toAddress");
    if (toAddressUni.isStr())
        this->ui->toLabel->setText(QString::fromStdString(toAddressUni.get_str()));

    UniValue amountUni = find_value(proposalData, "amount");
    if (amountUni.isNum())
        this->ui->amountLabel->setText(QString::number(amountUni.get_int()));

    UniValue feeUni = find_value(proposalData, "fee");
    if (feeUni.isNum())
        this->ui->feeLabel->setText(QString::number(feeUni.get_int()));

    UniValue actions = find_value(proposalData, "actions");
}

void PaymentProposal::acceptPressed()
{
    emit processProposal(proposalData, ProposalActionTypeAccept);
}

void PaymentProposal::rejectPressed()
{
    emit processProposal(proposalData, ProposalActionTypeReject);
}

PaymentProposal::~PaymentProposal()
{
    delete ui;
}
