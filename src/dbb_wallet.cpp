// Copyright (c) 2015 Jonas Schnelli
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "dbb_wallet.h"

#include "dbb_util.h"

void DBBWallet::updateData(const UniValue& walletResponse)
{
    availableBalance = 0;
    totalBalance = -1;
    walletRemoteName = "";
    currentPaymentProposals = UniValue(UniValue::VNULL);

    // check if error code exists
    UniValue code = find_value(walletResponse, "code");
    if (code.isStr())
        return;
    else 
        if(!client.walletJoined) {
            client.walletJoined = true;
            client.SaveLocalData();
        }

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

bool DBBWallet::rewriteKeypath(std::string& keypath)
{
    DBB::strReplace(keypath, "m", baseKeypath());
    return true;
}

void DBBWallet::setBaseKeypath(const std::string& keypath)
{
    std::unique_lock<std::recursive_mutex> lock(this->cs_wallet);
    _baseKeypath = keypath;
}

const std::string& DBBWallet::baseKeypath()
{
    std::unique_lock<std::recursive_mutex> lock(this->cs_wallet);
    return _baseKeypath;
}

void DBBWallet::setBackendURL(const std::string& backendUrl)
{
    client.setBaseURL(backendUrl);
}

void DBBWallet::setCAFile(const std::string& ca_file_in)
{
    client.setCAFile(ca_file_in);
}

void DBBWallet::setSocks5ProxyURL(const std::string& proxyURL)
{
    client.setSocks5ProxyURL(proxyURL);
}
