// Copyright (c) 2015 Jonas Schnelli
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "bpwalletclient.h"

#include <assert.h>
#include <string.h>

#include "util.h"



//ignore osx depracation warning
#pragma clang diagnostic ignored "-Wdeprecated-declarations"

BitPayWalletClient::BitPayWalletClient()
{

}

BitPayWalletClient::~BitPayWalletClient()
{
}

CKey BitPayWalletClient::GetNewKey()
{
    CKey key;
    key.MakeNewKey(true);
    return key;
}

bool BitPayWalletClient::ParseWalletInvitation(const std::string& walletInvitation, BitpayWalletInvitation& invitationOut)
{
    return true;
}