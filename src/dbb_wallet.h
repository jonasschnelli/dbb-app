// Copyright (c) 2015 Jonas Schnelli
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef DBB_WALLET_H
#define DBB_WALLET_H

#include "univalue.h"
#include "libbitpay-wallet-client/bpwalletclient.h"

class DBBWallet
{
public:
    BitPayWalletClient client;
    std::string baseKeyPath;
    std::string participationName;
    std::string walletRemoteName;
    UniValue currentPaymentProposals;
    int64_t totalBalance;
    int64_t availableBalance;
    DBBWallet()
    {
        baseKeyPath = "m/131'";
        participationName = "digitalbitbox";
    }

    /* synchronous post signatures for a given paymant proposal (accepts payment proposal) */
    void postSignaturesForPaymentProposal(const UniValue& proposal, const std::vector<std::string>& vSigs);

    /* tells the wallet server that it should broadcast the given payment proposal */
    void broadcastPaymentProposal(const UniValue& proposal);

    /* update wallet data from a getwallet json response */
    void updateData(const UniValue& walletResponse);
};


#endif