// Copyright (c) 2015 Jonas Schnelli
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "bpwalletclient.h"

#include <assert.h>
#include <string.h>

#include "util.h"



//ignore osx depracation warning
#pragma clang diagnostic ignored "-Wdeprecated-declarations"

extern void doubleSha256(char* string, unsigned char* hashOut);
static secp256k1_context_t* secp256k1_context = NULL;

BitPayWalletClient::BitPayWalletClient()
{
    InitECContext();
}

BitPayWalletClient::~BitPayWalletClient()
{
}

void BitPayWalletClient::InitECContext()
{
    secp256k1_context = secp256k1_context_create(SECP256K1_CONTEXT_SIGN | SECP256K1_CONTEXT_VERIFY);
    unsigned char seed[32];
    RAND_bytes(seed, 32);
    secp256k1_context_randomize(secp256k1_context, seed);
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