// Copyright (c) 2015 Jonas Schnelli
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef DBB_GETADDRESSDIALOG_H
#define DBB_GETADDRESSDIALOG_H

#include <QWidget>

#include <univalue.h>

namespace Ui {
    class GetAddressDialog;
}

class GetAddressDialog : public QWidget
{
    Q_OBJECT

public:
    explicit GetAddressDialog(QWidget *parent = 0);
    ~GetAddressDialog();

signals:
    void shouldGetXPub(const QString& keypath);

public slots:
    void addressBaseDataChanged();
    void updateAddress(const UniValue &xpubResult);

private:
    Ui::GetAddressDialog *ui;
    QString getCurrentKeypath();
};

#endif // DBB_GETADDRESSDIALOG_H
