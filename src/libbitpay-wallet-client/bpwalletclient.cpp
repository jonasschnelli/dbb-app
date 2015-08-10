// Copyright (c) 2015 Jonas Schnelli
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "bpwalletclient.h"

#include "config/_dbb-config.h"

#include <algorithm>

#include <assert.h>
#include <string.h>

#include "base58.h"
#include "eccryptoverify.h"
#include "keystore.h"
#include "serialize.h"
#include "streams.h"
#include "pubkey.h"
#include "univalue/univalue.h"
#include "util.h"
#include "utilstrencodings.h"

#include "libdbb/crypto.h"
#include "dbb_util.h"

#include <boost/filesystem.hpp>

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

void BitPayWalletClient::setPubKeys(const std::string& xPubKeyRequestKeyEntropy, const std::string& xPubKey)
{
    //set the extended public key from the key chain
    CBitcoinExtPubKey b58keyDecodeCheckXPubKey(xPubKey);
    CExtPubKey checkKey = b58keyDecodeCheckXPubKey.GetKey();
    masterPubKey = checkKey;
    
    //now this is a ugly workaround because we need a request keypair (pub/priv)
    //for signing the requests after BitAuth
    //Signing over the hardware wallet would be a very bad UX (press button on
    // every request) and it would be slow
    //the request key should be deterministic and linked to the master key
    //
    //we now generate a private key by (miss)using the xpub at m/1'/0' as entropy
    //for a new private key
    CBitcoinExtPubKey requestXPub(xPubKeyRequestKeyEntropy);
    CExtPubKey requestEntropyKey = requestXPub.GetKey();
    std::vector<unsigned char> data;
    data.resize(72);
    requestEntropyKey.Encode(&data[0]);

    CKeyingMaterial vSeed;
    vSeed.resize(32);
    int shift = 0;
    int cnt = 0;
    do {
        memcpy(&vSeed[0], (void *)(&data[0]+shift), 32);
        printf("current hex: %s\n", HexStr(vSeed, false).c_str());
        printf("seed round: %d shift: %d \n", cnt, shift);
        shift++;

        if (shift+32 >= data.size())
        {
            printf("reverse\n");
            shift = 0;
            //might turn into a endless loop
            std::reverse(data.begin(), data.end()); //do some more deterministic byte shuffeling
        }
    } while (!eccrypto::Check(&vSeed[0]));
    
    requestKey.Set(vSeed.begin(), vSeed.end(), true);
    
    SaveLocalData();
}

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
    printf("seed: request key: %s\n", HexStr(requestKey.begin(),requestKey.end(), false).c_str());
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

    long httpStatusCode = 0;
    SendRequest("post", "/v1/wallets/" + invitation.walletID + "/copayers", json, response, httpStatusCode);

    if (httpStatusCode != 200)
        return false;

    return true;
}

std::string BitPayWalletClient::SignRequest(const std::string& method,
                                            const std::string& url,
                                            const std::string& args)
{
    std::string message = method + "|" + url + "|" + args;
    uint256 hash = Hash(message.begin(), message.end());
    std::vector<unsigned char> signature;
    printf("signing request key: %s\n", HexStr(requestKey.begin(),requestKey.end(), false).c_str());
    requestKey.Sign(hash, signature);
    return HexStr(signature);
};

static size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp)
{
    ((std::string*)userp)->append((char*)contents, size * nmemb);
    return size * nmemb;
}

bool BitPayWalletClient::SendRequest(const std::string& method,
                                            const std::string& url,
                                            const std::string& args,
                                            std::string& responseOut,
                                            long& httpcodeOut)
{
    CURL* curl;
    CURLcode res;

    bool error = false;
    
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
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &responseOut);

#ifdef DBB_ENABLE_DEBUG
        curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
#endif

        res = curl_easy_perform(curl);
        if (res != CURLE_OK)
        {
            fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
            error = true;
        }
        else
        {
            curl_easy_getinfo (curl, CURLINFO_RESPONSE_CODE, &httpcodeOut);
            if (httpcodeOut != 200)
                error = true;
        }

        curl_easy_cleanup(curl);
    }
    curl_global_cleanup();

#ifdef DBB_ENABLE_DEBUG
    printf("response: %s", responseOut.c_str());
#endif

    return error;
};


boost::filesystem::path GetDefaultDBBDataDir()
{
    namespace fs = boost::filesystem;
    // Windows < Vista: C:\Documents and Settings\Username\Application Data\Bitcoin
    // Windows >= Vista: C:\Users\Username\AppData\Roaming\Bitcoin
    // Mac: ~/Library/Application Support/Bitcoin
    // Unix: ~/.bitcoin
#ifdef WIN32
    // Windows
    return GetSpecialFolderPath(CSIDL_APPDATA) / "DBB";
#else
    fs::path pathRet;
    char* pszHome = getenv("HOME");
    if (pszHome == NULL || strlen(pszHome) == 0)
        pathRet = fs::path("/");
    else
        pathRet = fs::path(pszHome);
#ifdef MAC_OSX
    // Mac
    pathRet /= "Library/Application Support";
    TryCreateDirectory(pathRet);
    return pathRet / "DBB";
#else
    // Unix
    return pathRet / ".dbb";
#endif
#endif
}

bool BitPayWalletClient::IsSeeded()
{
    if (requestKey.IsValid())
        return true;
    
    return false;
}

void BitPayWalletClient::SaveLocalData()
{
    boost::filesystem::path dataDir = GetDefaultDBBDataDir();
    boost::filesystem::create_directories(dataDir);
    FILE *writeFile = fopen( (dataDir / "copay.dat").string().c_str(), "wb");
    if (writeFile)
    {
        CAutoFile copayDatFile(writeFile, SER_DISK, 1);
        copayDatFile << requestKey.GetPrivKey();
        
        CKeyingMaterial encoded;
        encoded.resize(72);

        masterPubKey.Encode(&encoded[0]);
        copayDatFile << encoded;
    }
    fclose(writeFile);
}

void BitPayWalletClient::LoadLocalData()
{
    boost::filesystem::path dataDir = GetDefaultDBBDataDir();
    boost::filesystem::create_directories(dataDir);
    FILE* fh = fopen( (dataDir / "copay.dat").string().c_str(), "rb");
    if (fh)
    {
        CAutoFile copayDatFile(fh, SER_DISK, 1);
        if (!copayDatFile.IsNull())
        {
            CPrivKey pkey;
            copayDatFile >> pkey;
            requestKey.SetPrivKey(pkey, false);
            
            CKeyingMaterial encoded;
            encoded.resize(72);
            copayDatFile >> encoded;
            
            masterPubKey.Decode(&encoded[0]);
        }
        fclose(fh);
    }
}