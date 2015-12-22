// Copyright (c) 2015 Jonas Schnelli / Shift Devices AG
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "signconfirmationdialog.h"
#include "ui/ui_signconfirmationdialog.h"

#include "dbb_util.h"

SignConfirmationDialog::SignConfirmationDialog(QWidget* parent) : QWidget(parent),
                                          ui(new Ui::SignConfirmationDialog)
{
    ui->setupUi(this);

    //ui->amountLabelKey->setStyleSheet("font-weight: bold;");
    //ui->feeLabelKey->setStyleSheet("font-weight: bold;");
    ui->smartphoneInfo->setStyleSheet("font-weight: bold; color: rgb(0,0,100);");
    ui->titleLabel->setStyleSheet("font-weight:bold; font-size: 20pt;");
}

void SignConfirmationDialog::setData(const UniValue &dataIn)
{
    data = dataIn;

    QString longString;

    longString += "Do you want to send ";

    UniValue amountUni = find_value(data, "amount");
    if (amountUni.isNum())
    {
        longString += "<strong>"+QString::fromStdString(DBB::formatMoney(amountUni.get_int64()))+"</strong>";
    }

    UniValue toAddressUni = find_value(data, "toAddress");
    if (toAddressUni.isStr())
    {
        longString += " to <strong>"+QString::fromStdString(toAddressUni.get_str())+"</strong>";
    }

    UniValue feeUni = find_value(data, "fee");
    if (feeUni.isNum())
    {
        longString += " with an additional fee of <strong>" + QString::fromStdString(DBB::formatMoney(feeUni.get_int64()))+" BTC</strong>";
    }

    ui->longTextLabel->setText(longString);
}

SignConfirmationDialog::~SignConfirmationDialog()
{
    delete ui;
}
