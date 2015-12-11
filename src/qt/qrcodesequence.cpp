// Copyright (c) 2015 Jonas Schnelli / Shift Devices AG
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "qrcodesequence.h"
#include "ui/ui_qrcodesequence.h"

#include <cmath>

QRCodeSequence::QRCodeSequence(QWidget* parent) : QWidget(parent), ui(new Ui::QRCodeSequence)
{
    ui->setupUi(this);

    connect(ui->nextButton, SIGNAL(clicked()), this, SLOT(nextButton()));
    connect(ui->prevButton, SIGNAL(clicked()), this, SLOT(prevButton()));
}

QRCodeSequence::~QRCodeSequence()
{
    removeQRcodes();
    delete ui;
}

void QRCodeSequence::removeQRcodes()
{
    for (QRcode *qrcode : qrcodes)
    {
        QRcode_free(qrcode);
    }
    qrcodes.clear();
}

void QRCodeSequence::nextButton()
{
    if (currentPage < qrcodes.size()-1)
        showPage(currentPage+1);
}

void QRCodeSequence::prevButton()
{
    if (currentPage > 0)
        showPage(currentPage-1);
}

void QRCodeSequence::showPage(int page)
{
    if (page >= qrcodes.size())
        return;

    QRcode *code = qrcodes[page];
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

        QIcon qrCode;
        QPixmap pixMap = QPixmap::fromImage(myImage).scaled(240, 240);
        qrCode.addPixmap(pixMap, QIcon::Normal);
        qrCode.addPixmap(pixMap, QIcon::Disabled);
        ui->qrCode->setIcon(qrCode);
        ui->qrCode->setIconSize(QSize(ui->qrCode->width(), ui->qrCode->height()));
    }

    ui->pageLabel->setText(QString::number(page+1)+"/"+QString::number(qrcodes.size()));
    currentPage = page;
}

void QRCodeSequence::setData(const std::string& data)
{
    removeQRcodes();
    
    size_t maxSize = data.size();
    int numPages = ceil(maxSize/QRCODE_MAX_CHARS);

    // for now, don't allow more then 9 pages
    if (numPages > 10)
        return;

    int i;
    for (i=0;i<numPages;i++)
    {
        size_t pageSize = QRCODE_MAX_CHARS;
        if (i == numPages-1)
            pageSize = maxSize - (numPages-1) * QRCODE_MAX_CHARS;

        std::string subStr = data.substr(i*QRCODE_MAX_CHARS, pageSize);

        if (numPages > 1)
            subStr = QRCODE_SEQUENCE_HEADER0 + std::to_string(i) + std::to_string(numPages) + subStr ;

        QRcode *code = QRcode_encodeString(subStr.c_str(), 0, QR_ECLEVEL_L, QR_MODE_8, 1);
        qrcodes.push_back(code);
    }

    showPage(0);
}