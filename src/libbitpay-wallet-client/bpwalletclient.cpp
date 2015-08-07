// Copyright (c) 2015 Jonas Schnelli
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "bpwalletclient.h"

#include "config/_dbb-config.h"

#include <assert.h>
#include <string.h>

#include "base58.h"
#include "eccryptoverify.h"
#include "keystore.h"
#include "univalue/univalue.h"
#include "util.h"
#include "utilstrencodings.h"

#include "libdbb/crypto.h"


//ignore osx depracation warning
#pragma clang diagnostic ignored "-Wdeprecated-declarations"

BitPayWalletClient::BitPayWalletClient()
{
    SelectParams(CBaseChainParams::MAIN);
    baseURL = "https://bws.bitpay.com/bws/api";
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


std::vector<std::string> BitPayWalletClient::split(const std::string& str, std::vector<int> indexes)
{
    std::vector<std::string> parts;
    indexes.push_back(str.size());
    int i = 0;
    while (i < indexes.size()) {
        int from = i == 0 ? 0 : indexes[i - 1];
        parts.push_back(str.substr(from, indexes[i] - from));
        i++;
    };
    return parts;
};

std::string BitPayWalletClient::_copayerHash(const std::string& name, const std::string& xPubKey, const std::string& requestPubKey)
{
    return name + "|" + xPubKey + "|" + requestPubKey;
};

std::string BitPayWalletClient::GetXPubKey()
{
    CBitcoinExtPubKey xpubkey;
    xpubkey.SetKey(masterPubKey);
    return xpubkey.ToString();
}
bool BitPayWalletClient::GetCopayerHash(const std::string& name, std::string& out)
{
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

bool BitPayWalletClient::GetCopayerSignature(const std::string& stringToHash, const CKey& privKey, std::string& sigHexOut)
{
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
    masterPrivKeyRoot.Derive(masterPrivKey, 45);          //m/45' xpriv
    masterPubKey = masterPrivKey.Neuter();                //m/45' xpub

    CExtKey requestKeyChain;
    masterPrivKeyRoot.Derive(requestKeyChain, 1); //m/1'

    CExtKey requestKeyExt;
    requestKeyChain.Derive(requestKeyExt, 0);

    requestKey = requestKeyExt.key;
}

bool BitPayWalletClient::GetRequestPubKey(std::string& pubKeyOut)
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
    invitationOut.walletID = walletIdParts[0] + "-" + walletIdParts[1] + "-" + walletIdParts[2] + "-" + walletIdParts[3] + "-" + walletIdParts[4];

    std::string walletPrivKeyStr = secretSplit[1];
    CBitcoinSecret vchSecret;
    if (!vchSecret.SetString(walletPrivKeyStr))
        return false;

    invitationOut.walletPrivKey = vchSecret.GetKey();
    invitationOut.network = secretSplit[2] == "T" ? "testnet" : "livenet";
    return true;
}

bool BitPayWalletClient::JoinWallet(const std::string& name, const std::string& walletInvitation, std::string& response)
{
    std::string requestPubKey;
    if (!GetRequestPubKey(requestPubKey))
        return false;

    BitpayWalletInvitation invitation;
    if (!ParseWalletInvitation(walletInvitation, invitation))
        return false;

    std::string copayerHash;
    if (!GetCopayerHash(name, copayerHash))
        return false;

    std::string copayerSignature;
    if (!GetCopayerSignature(copayerHash, invitation.walletPrivKey, copayerSignature))
        return false;

    //form request
    UniValue jsonArgs(UniValue::VOBJ);
    jsonArgs.push_back(Pair("walletId", invitation.walletID));
    jsonArgs.push_back(Pair("name", name));
    jsonArgs.push_back(Pair("xPubKey", GetXPubKey()));
    jsonArgs.push_back(Pair("requestPubKey", requestPubKey));
    jsonArgs.push_back(Pair("isTemporaryRequestKey", false));
    jsonArgs.push_back(Pair("copayerSignature", copayerSignature));
    std::string json = jsonArgs.write();

    response = SendRequest("post", "/v1/wallets/" + invitation.walletID + "/copayers", json);
    //TODO check response

    return true;
}

std::string BitPayWalletClient::SignRequest(const std::string& method,
                                            const std::string& url,
                                            const std::string& args)
{
    std::string message = method + "|" + url + "|" + args;
    uint256 hash = Hash(message.begin(), message.end());
    std::vector<unsigned char> signature;
    requestKey.Sign(hash, signature);
    return HexStr(signature);
};

static size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp)
{
    ((std::string*)userp)->append((char*)contents, size * nmemb);
    return size * nmemb;
}

std::string BitPayWalletClient::SendRequest(const std::string& method,
                                            const std::string& url,
                                            const std::string& args)
{
    CURL* curl;
    CURLcode res;
    std::string writeBuffer;

    curl_global_init(CURL_GLOBAL_ALL);
    curl = curl_easy_init();
    if (curl) {
        struct curl_slist* chunk = NULL;
        std::string requestPubKey;
        GetRequestPubKey(requestPubKey);
        std::string signature = SignRequest(method, url, args);
        chunk = curl_slist_append(chunk, ("x-identity: " + requestPubKey).c_str());
        chunk = curl_slist_append(chunk, ("x-signature: " + signature).c_str());
        chunk = curl_slist_append(chunk, "Content-Type: application/json");
        res = curl_easy_setopt(curl, CURLOPT_HTTPHEADER, chunk);
        curl_easy_setopt(curl, CURLOPT_URL, (baseURL + url).c_str());
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, args.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &writeBuffer);

#ifdef DBB_ENABLE_DEBUG
        curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
#endif

        res = curl_easy_perform(curl);
        if (res != CURLE_OK)
            fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));

        curl_easy_cleanup(curl);
    }
    curl_global_cleanup();

#ifdef DBB_ENABLE_DEBUG
    printf("response: %s", writeBuffer.c_str());
#endif

    return writeBuffer;
};