// Copyright (c) 2015 Jonas Schnelli
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "dbb_wallet.h"


void DBBWallet::broadcastPaymentProposal(const UniValue& proposal)
{
    client.BroadcastProposal(proposal);
}

void DBBWallet::postSignaturesForPaymentProposal(const UniValue& proposal, const std::vector<std::string>& vSigs)
{
    client.PostSignaturesForTxProposal(proposal, vSigs);
}

void DBBWallet::updateData(const UniValue& walletResponse)
{
    availableBalance = 0;
    walletRemoteName = "";
    currentPaymentProposals = UniValue(UniValue::VNULL);

    // get balance and name
    UniValue balanceObj = find_value(walletResponse, "balance");
    if (balanceObj.isObject()) {
        UniValue availableAmountUni = find_value(balanceObj, "availableAmount");
        if (availableAmountUni.isNum())
            availableBalance = availableAmountUni.get_int64();

        UniValue totalAmountUni = find_value(balanceObj, "totalAmount");
        if (totalAmountUni.isNum())
            totalBalance = totalAmountUni.get_int64();
    }

    UniValue walletObj = find_value(walletResponse, "wallet");
    if (walletObj.isObject()) {
        UniValue nameUni = find_value(walletObj, "name");
        if (nameUni.isStr())
            walletRemoteName = nameUni.get_str();


        std::string mStr;
        std::string nStr;

        UniValue mUni = find_value(walletObj, "m");
        if (mUni.isNum())
            mStr = std::to_string(mUni.get_int());

        UniValue nUni = find_value(walletObj, "n");
        if (nUni.isNum())
            nStr = std::to_string(nUni.get_int());

        if (mStr.size() > 0 && nStr.size() > 0)
            walletRemoteName += " (" + mStr + " of " + nStr + ")";
    }

    UniValue pendingTxps = find_value(walletResponse, "pendingTxps");
    if (pendingTxps.isArray())
        currentPaymentProposals = pendingTxps;
}