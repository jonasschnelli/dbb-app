// Copyright (c) 2015 Jonas Schnelli
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef DBB_VERIFICATIONDIALOG_H
#define DBB_VERIFICATIONDIALOG_H

#include <QDialog>
#include "qrcodesequence.h"

namespace Ui {
    class VerificationDialog;
}

class VerificationDialog : public QDialog
{
    Q_OBJECT

public:
    explicit VerificationDialog(QWidget *parent = 0);
    ~VerificationDialog();
    void setData(const QString& title, const QString& detailText, const std::string& qrCodeData);

private:
    Ui::VerificationDialog *ui;
};

#endif // DBB_VERIFICATIONDIALOG_H
