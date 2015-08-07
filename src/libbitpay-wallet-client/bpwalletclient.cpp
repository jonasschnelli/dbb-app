// Copyright (c) 2015 Jonas Schnelli
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "bpwalletclient.h"

#include <assert.h>
#include <string.h>

#include "base58.h"
#include "eccryptoverify.h"
#include "keystore.h"
#include "util.h"
#include "utilstrencodings.h"

#include "libdbb/crypto.h"


//ignore osx depracation warning
#pragma clang diagnostic ignored "-Wdeprecated-declarations"

BitPayWalletClient::BitPayWalletClient()
{
    SelectParams(CBaseChainParams::MAIN);
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


std::vector<std::string> BitPayWalletClient::split(const std::string& str, std::vector<int> indexes) {
    std::vector<std::string> parts;
    indexes.push_back(str.size());
    int i = 0;
    while (i < indexes.size()) {
        int from = i == 0 ? 0 : indexes[i - 1];
        parts.push_back(str.substr(from, indexes[i]-from));
        i++;
    };
    return parts;
};

std::string BitPayWalletClient::_copayerHash(const std::string& name, const std::string& xPubKey, const std::string& requestPubKey) {
    return name+"|"+xPubKey+"|"+requestPubKey;
};

std::string BitPayWalletClient::GetXPubKey()
{
    CBitcoinExtPubKey xpubkey;
    xpubkey.SetKey(masterPubKey);
    return xpubkey.ToString();
}
bool BitPayWalletClient::GetCopayerHash(const std::string& name, std::string& out) {
    if (!requestKey.IsValid())
        return false;
    
    CBitcoinExtPubKey xpubkey;
    xpubkey.SetKey(masterPubKey);
    
    std::string requestKeyHex;
    if (!GetRequestPubKey(requestKeyHex))
        return false;
    
    out = _copayerHash(name, xpubkey.ToString(), requestKeyHex);
    return true;
};

bool BitPayWalletClient::GetCopayerSignature(const std::string& stringToHash, const CKey& privKey, std::string& sigHexOut) {
    uint256 hash = Hash(stringToHash.begin(), stringToHash.end());
    std::vector<unsigned char> signature;
    privKey.Sign(hash, signature);
    
    sigHexOut = HexStr(signature);
    return true;
};

void BitPayWalletClient::seed()
{
    CKeyingMaterial vSeed;
    vSeed.resize(32);

    RandAddSeedPerfmon();
    do {
        GetRandBytes(&vSeed[0], vSeed.size());
    } while (!eccrypto::Check(&vSeed[0]));

    CExtKey masterPrivKeyRoot;
    masterPrivKeyRoot.SetMaster(&vSeed[0], vSeed.size()); //m
    masterPrivKeyRoot.Derive(masterPrivKey, 45); //m/45' xpriv
    masterPubKey = masterPrivKey.Neuter(); //m/45' xpub
    
    CExtKey requestKeyChain;
    masterPrivKeyRoot.Derive(requestKeyChain, 1); //m/1'
    
    CExtKey requestKeyExt;
    requestKeyChain.Derive(requestKeyExt, 0);
    
    requestKey = requestKeyExt.key;
}
    
bool BitPayWalletClient::GetRequestPubKey(std::string &pubKeyOut)
{
    if (!requestKey.IsValid())
        return false;
            
    pubKeyOut = HexStr(requestKey.GetPubKey(), false);
    return true;
}

bool BitPayWalletClient::ParseWalletInvitation(const std::string& walletInvitation, BitpayWalletInvitation& invitationOut)
{
    std::vector<int> splits = {22, 74};
    std::vector<std::string> secretSplit = split(walletInvitation, splits);
    
    //TODO: var widBase58 = secretSplit[0].replace(/0/g, '');
    std::string widBase58 = secretSplit[0];
    std::vector<unsigned char> vch;
    if (!DecodeBase58(widBase58.c_str(), vch))
        return false;
    
    std::string widHex = HexStr(vch, false);

    splits = {8, 12, 16, 20};
    std::vector<std::string> walletIdParts = split(widHex, splits);
    invitationOut.walletID = walletIdParts[0]+"-"+walletIdParts[1]+"-"+walletIdParts[2]+"-"+walletIdParts[3]+"-"+walletIdParts[4];
    
    std::string walletPrivKeyStr = secretSplit[1];
    CBitcoinSecret vchSecret;
    if (!vchSecret.SetString(walletPrivKeyStr))
        return false;

    invitationOut.walletPrivKey = vchSecret.GetKey();
    invitationOut.network = secretSplit[2] == "T" ? "testnet" : "livenet";
    return true;
}

std::string BitPayWalletClient::SignRequest(const std::string& method,
                                                 const std::string& url,
                                                 const std::string& args)
{
  std::string message = method+"|"+url+"|"+args;
  uint256 hash = Hash(message.begin(), message.end());
  std::vector<unsigned char> signature;
  requestKey.Sign(hash, signature);
  return HexStr(signature);
};