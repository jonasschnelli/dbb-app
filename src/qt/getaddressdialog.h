// Copyright (c) 2015 Jonas Schnelli
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef DBB_GETADDRESSDIALOG_H
#define DBB_GETADDRESSDIALOG_H

#include <QDialog>

#include <univalue.h>

namespace Ui {
    class GetAddressDialog;
}

class GetAddressDialog : public QDialog
{
    Q_OBJECT

public:
    explicit GetAddressDialog(QWidget *parent = 0);
    ~GetAddressDialog();

signals:
    void shouldGetXPub(const QString& keypath);

public slots:
    void addressBaseDataChanged();
    void setLoading(bool state);
    void updateAddress(const UniValue &xpubResult);
    void keypathEditFinished();

private:
    Ui::GetAddressDialog *ui;
    QString getCurrentKeypath();
    QString lastKeypath;
    void showEvent(QShowEvent * event);
};

#endif // DBB_GETADDRESSDIALOG_H
