// Copyright (c) 2015 Jonas Schnelli / Shift Devices AG
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "getaddressdialog.h"
#include "ui/ui_getaddressdialog.h"

#include <btc/bip32.h>
#include <btc/ecc_key.h>
#include <btc/base58.h>
#include <qrencode.h>

#define DBB_DEFAULT_KEYPATH "m/44'/0'/0'/0/"

GetAddressDialog::GetAddressDialog(QWidget *parent) :
QDialog(parent),
ui(new Ui::GetAddressDialog), _baseKeypath(DBB_DEFAULT_KEYPATH)
{
    ui->setupUi(this);

    ui->address->setFrame(false);

    //connect buttons to slots
    connect(ui->kpDefault, SIGNAL(clicked()), this, SLOT(addressBaseDataChanged()));
    connect(ui->kpCustom, SIGNAL(clicked()), this, SLOT(addressBaseDataChanged()));
    connect(ui->childIndex, SIGNAL(valueChanged(int)), this, SLOT(addressBaseDataChanged()));
    connect(ui->keypath, SIGNAL(editingFinished()), this, SLOT(keypathEditFinished()));
    connect(ui->verifyGetAddressesButton, SIGNAL(clicked()), this, SLOT(verifyGetAddressButtonPressed()));
}

void GetAddressDialog::verifyGetAddressButtonPressed()
{
    emit verifyGetAddress(getCurrentKeypath());
}

void GetAddressDialog::showEvent(QShowEvent * event)
{
    addressBaseDataChanged();
}

void GetAddressDialog::addressBaseDataChanged()
{
    QString newKeypath = getCurrentKeypath();
    if (newKeypath.compare(lastKeypath) != 0)
    {
        emit shouldGetXPub(getCurrentKeypath());
        setLoading(true);
        lastKeypath = newKeypath;
    }
}

void GetAddressDialog::keypathEditFinished()
{
    if (!ui->keypath->isReadOnly() && ui->kpDefault->isEnabled())
        addressBaseDataChanged();
}

void GetAddressDialog::setLoading(bool state)
{
    if (state)
    {
        ui->xpub->setText("");
        ui->address->setText("");
    }

    ui->kpDefault->setDisabled(state);
    ui->kpCustom->setDisabled(state);
    ui->childIndex->setDisabled(state);

    ui->keypath->setReadOnly(true);

    if (!state && ui->kpCustom->isChecked())
    {
        ui->keypath->setReadOnly(false);
        ui->childIndex->setDisabled(true);
    }
}

void GetAddressDialog::updateAddress(const UniValue &xpub)
{
    setLoading(false);

    if (!xpub.isObject())
        return;

    UniValue xpubUV = find_value(xpub, "xpub");
    if (xpubUV.isStr())
    {
        ui->xpub->setText(QString::fromStdString(xpubUV.get_str()));

        btc_hdnode node;
        bool r = btc_hdnode_deserialize(xpubUV.get_str().c_str(), &btc_chain_main, &node);
        char outbuf[112];
        btc_hdnode_get_p2pkh_address(&node, &btc_chain_main, outbuf, sizeof(outbuf));

        ui->address->setText(QString::fromUtf8(outbuf));

        std::string uri = "bitcoin:";
        uri.append(outbuf);

        QRcode *code = QRcode_encodeString(uri.c_str(), 0, QR_ECLEVEL_M, QR_MODE_8, 1);
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

            QPixmap pixMap = QPixmap::fromImage(myImage).scaled(180, 180);
            ui->qrCodeGetAddresses->setPixmap(pixMap);
            ui->qrCodeGetAddresses->setText("");
        }
    }
}

QString GetAddressDialog::getCurrentKeypath()
{
    ui->childIndex->setDisabled(false);
    
    if (ui->kpCustom->isChecked())
    {
        ui->childIndex->setDisabled(true);
        ui->keypath->setReadOnly(false);
        QString kp = ui->keypath->text();
        if (kp == "m")
            kp = "m/";
        return kp;
    }
    ui->keypath->setReadOnly(true);

    QString basePath;
    if (ui->kpDefault->isChecked())
        basePath = QString::fromStdString(_baseKeypath);

    basePath += ui->childIndex->text();
    ui->keypath->setText(basePath);
    return basePath;
}

void GetAddressDialog::setBaseKeypath(const std::string& keypath)
{
    _baseKeypath = keypath;
    if(_baseKeypath.back() != '/')
        _baseKeypath += "/0/";
}

GetAddressDialog::~GetAddressDialog()
{
    delete ui;
}
