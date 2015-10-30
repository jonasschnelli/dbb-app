// Copyright (c) 2015 Jonas Schnelli
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef DBB_PAYMENTPROPOSAL_H
#define DBB_PAYMENTPROPOSAL_H

#include <QWidget>
#include <univalue.h>

enum ProposalActionType
{
    ProposalActionTypeAccept,
    ProposalActionTypeReject
};

namespace Ui {
    class PaymentProposal;
}

class PaymentProposal : public QWidget
{
    Q_OBJECT

signals:
    void processProposal(const UniValue &proposalData, int actionType);

public slots:
    void acceptPressed();
    void rejectPressed();

public:
    explicit PaymentProposal(QWidget *parent = 0);
    ~PaymentProposal();
    Ui::PaymentProposal *ui;
    void SetData(const UniValue &data);
private:
    UniValue proposalData;
};

#endif // DBB_PAYMENTPROPOSAL_H
