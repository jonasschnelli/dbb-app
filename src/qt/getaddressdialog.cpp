// Copyright (c) 2015 Jonas Schnelli / Shift Devices AG
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "getaddressdialog.h"
#include "ui/ui_getaddressdialog.h"

#include <btc/bip32.h>
#include <btc/ecc_key.h>
#include <btc/base58.h>

GetAddressDialog::GetAddressDialog(QWidget *parent) :
QWidget(parent),
ui(new Ui::GetAddressDialog)
{
    ui->setupUi(this);

    ui->customKeypathLabel->setVisible(false);
    ui->customKeypath->setVisible(false);

    //connect buttons to slots
    connect(ui->kpDefault, SIGNAL(clicked()), this, SLOT(addressBaseDataChanged()));
    connect(ui->kpMK, SIGNAL(clicked()), this, SLOT(addressBaseDataChanged()));
    connect(ui->kpCustom, SIGNAL(clicked()), this, SLOT(addressBaseDataChanged()));
}
void GetAddressDialog::addressBaseDataChanged()
{
    emit shouldGetXPub(getCurrentKeypath());
}

void GetAddressDialog::updateAddress(const UniValue &xpub)
{
    if (!xpub.isObject())
        return;

    UniValue xpubUV = find_value(xpub, "xpub");
    if (xpubUV.isStr())
    {
        ui->xpub->setText(QString::fromStdString(xpubUV.get_str()));

        btc_hdnode node;
        bool r = btc_hdnode_deserialize(xpubUV.get_str().c_str(), &btc_chain_main, &node);
        btc_pubkey pubkey;
        btc_pubkey_init(&pubkey);
        memcpy(&pubkey.pubkey, &node.public_key, BTC_ECKEY_COMPRESSED_LENGTH);

        uint8_t hash160[21];
        hash160[0] = 0;
        btc_pubkey_get_hash160(&pubkey, hash160+1);
        char adrOut[128];
        btc_base58_encode_check(hash160, 21, adrOut, 128);

        char outbuf[112];
        btc_hdnode_serialize_public(&node, &btc_chain_main, outbuf, sizeof(outbuf));

        std::string xPubKeyNew(outbuf);

    }
}

QString GetAddressDialog::getCurrentKeypath()
{
    QString basePath;
    if (ui->kpDefault->isChecked())
        basePath = "m/44/";
    else if (ui->kpMK->isChecked())
        basePath = "m/";

    return basePath;
}

GetAddressDialog::~GetAddressDialog()
{
    delete ui;
}
