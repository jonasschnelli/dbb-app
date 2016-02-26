// Copyright (c) 2015 Jonas Schnelli
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef QRCODESEQUENCE_H
#define QRCODESEQUENCE_H

#define QRCODE_MAX_CHARS 200.0
#define QRCODE_SEQUENCE_HEADER0 "QS"

#include <QWidget>

#include <qrencode.h>

namespace Ui {
    class QRCodeSequence;
}

class QRCodeSequence : public QWidget
{
    Q_OBJECT

public:
    explicit QRCodeSequence(QWidget* parent = 0);
    ~QRCodeSequence();
    void setData(const std::string &data);

public slots:
    void nextButton();
    void prevButton();
    void showPage(int page = 0);
    void useOnDarkBackground(bool state);
    static void setIconFromQRCode(QRcode *qrcode, QIcon *icon, int width = 240, int height = 240);

private:
    Ui::QRCodeSequence *ui;
    std::vector<QRcode *>qrcodes;
    void removeQRcodes();
    int currentPage;
};

#endif // QRCODESEQUENCE_H