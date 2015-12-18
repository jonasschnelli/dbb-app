// Copyright (c) 2015 Jonas Schnelli
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef DBB_WALLET_H
#define DBB_WALLET_H

#include "univalue.h"
#include "libbitpay-wallet-client/bpwalletclient.h"

#include <atomic>
#include <mutex>
#ifdef WIN32
#include <windows.h>
#include "mingw/mingw.mutex.h"
#include "mingw/mingw.condition_variable.h"
#include "mingw/mingw.thread.h"
#endif

class DBBWallet
{
private:
    std::recursive_mutex cs_wallet;
    std::string _baseKeypath;

public:
    BitPayWalletClient client;
    std::string participationName;
    std::string walletRemoteName;
    UniValue currentPaymentProposals;
    int64_t totalBalance;
    int64_t availableBalance;
    std::atomic<bool> updatingWallet;
    std::atomic<bool> shouldUpdateWalletAgain;
    DBBWallet()
    {
        _baseKeypath = "m/131'/45'";
        participationName = "digitalbitbox";
        updatingWallet = false;
    }

    /* synchronous post signatures for a given paymant proposal (accepts payment proposal) */
    void postSignaturesForPaymentProposal(const UniValue& proposal, const std::vector<std::string>& vSigs);

    /* tells the wallet server that it should broadcast the given payment proposal */
    void broadcastPaymentProposal(const UniValue& proposal);

    /* update wallet data from a getwallet json response */
    void updateData(const UniValue& walletResponse);

    bool rewriteKeypath(std::string& keypath);

    void setBaseKeypath(const std::string& keypath);
    const std::string& baseKeypath();
};


#endif