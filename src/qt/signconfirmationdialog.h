// Copyright (c) 2015 Jonas Schnelli / Shift Devices AG
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <QWidget>

#include <univalue.h>

namespace Ui
{
class SignConfirmationDialog;
}

class SignConfirmationDialog : public QWidget
{
    Q_OBJECT

public:
    explicit SignConfirmationDialog(QWidget* parent = 0);
    ~SignConfirmationDialog();

    void setData(const UniValue &data);

private:
    Ui::SignConfirmationDialog* ui;
    UniValue data;
};

