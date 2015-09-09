// Copyright (c) 2015 Jonas Schnelli
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "bpwalletclient.h"

#include "config/_dbb-config.h"

#include <algorithm>

#include <assert.h>
#include <string.h>

#include "base58.h"
#include "core_io.h"
#include "eccryptoverify.h"
#include "keystore.h"
#include "serialize.h"
#include "streams.h"
#include "pubkey.h"
#include "util.h"
#include "utilstrencodings.h"

#include "libdbb/crypto.h"
#include "dbb_util.h"

#include <boost/filesystem.hpp>

//ignore osx depracation warning
#pragma clang diagnostic ignored "-Wdeprecated-declarations"

#include <climits>

std::string BitPayWalletClient::ReversePairs(std::string const& src)
{
    assert(src.size() % 2 == 0);
    std::string result;
    result.reserve(src.size());

    for (std::size_t i = src.size(); i != 0; i -= 2) {
        result.append(src, i - 2, 2);
    }

    return result;
}

BitPayWalletClient::BitPayWalletClient()
{
    SelectParams(CBaseChainParams::TESTNET);
    baseURL = "https://bws.bitpay.com/bws/api";
}

BitPayWalletClient::~BitPayWalletClient()
{
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
    printf("Set IN Master XPubKey: %s\n", xPubKey.c_str());
    //set the extended public key from the key chain
    CBitcoinExtPubKey b58keyDecodeCheckXPubKey(xPubKey);
    CExtPubKey checkKey = b58keyDecodeCheckXPubKey.GetKey();
    masterPubKey = checkKey;

    CBitcoinExtPubKey b58PubkeyDecodeCheck(masterPubKey);
    printf("Set Master XPubKey: %s\n", b58PubkeyDecodeCheck.ToString().c_str());

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
    data.resize(74);
    requestEntropyKey.Encode(&data[0]);

    CKeyingMaterial vSeed;
    vSeed.resize(32);
    int shift = 0;
    int cnt = 0;
    do {
        memcpy(&vSeed[0], (void*)(&data[0] + shift), 32);
        printf("current hex: %s\n", HexStr(vSeed, false).c_str());
        printf("seed round: %d shift: %d \n", cnt, shift);
        shift++;

        if (shift + 32 >= data.size()) {
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
    printf("seed: request key: %s\n", HexStr(requestKey.begin(), requestKey.end(), false).c_str());
}

bool BitPayWalletClient::GetRequestPubKey(std::string& pubKeyOut)
{
    if (!requestKey.IsValid())
        return false;

    pubKeyOut = HexStr(requestKey.GetPubKey(), false);
    return true;
}

std::string BitPayWalletClient::GetCopayerId()
{
    CBitcoinExtPubKey base58(masterPubKey);
    std::string output = base58.ToString();
    unsigned char rkey[32];
    CSHA256().Write((const unsigned char*)output.c_str(), output.size()).Finalize(rkey);
    return HexStr(rkey, rkey + DBB_SHA256_DIGEST_LENGTH);
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

    SelectParams(CBaseChainParams::MAIN);
    std::string walletPrivKeyStr = secretSplit[1];
    CBitcoinSecret vchSecret;
    if (!vchSecret.SetString(walletPrivKeyStr))
        return false;

    SelectParams(CBaseChainParams::TESTNET);
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


    std::string getWalletsResponse;
    GetWallets(getWalletsResponse);

    if (httpStatusCode != 200)
        return false;

    return true;
}

bool BitPayWalletClient::GetWallets(std::string& response)
{
    std::string requestPubKey;
    if (!GetRequestPubKey(requestPubKey))
        return false;

    long httpStatusCode = 0;
    SendRequest("get", "/v1/wallets/?r=16354", "{}", response, httpStatusCode);

    if (httpStatusCode != 200)
        return false;

    return true;
}

std::string BitPayWalletClient::ParseTxProposal(const UniValue& txProposal, std::vector<std::pair<std::string, uint256> >& vInputTxHashes)
{
    CMutableTransaction t;
    std::vector<std::string> keys = txProposal.getKeys();
    std::vector<UniValue> values = txProposal.getValues();

    std::string toAddress;
    CAmount toAmount = -1;
    CAmount fee = -1;
    std::vector<int> outputOrder;
    int requiredSignatures = -1;
    int i = 0;
    int j = 0;
    for (i = 0; i < keys.size(); i++) {
        UniValue val = values[i];

        if (keys[i] == "toAddress")
            toAddress = val.get_str();

        if (keys[i] == "amount")
            toAmount = val.get_int64();

        if (keys[i] == "fee")
            fee = val.get_int64();

        if (keys[i] == "outputOrder")
            for (UniValue aVal : val.getValues())
                outputOrder.push_back(aVal.get_int());

        if (keys[i] == "requiredSignatures")
            requiredSignatures = val.get_int();
    }

    UniValue inputsObj = find_value(txProposal, "inputs");
    std::vector<UniValue> inputs = inputsObj.getValues();
    CAmount inTotal = 0;

    CScript checkScript;
    std::vector<std::pair<std::string, CScript> > inputsScriptAndPath;
    for (i = 0; i < inputs.size(); i++) {
        UniValue aInput = inputs[i];
        std::vector<std::string> keys = aInput.getKeys();
        std::vector<UniValue> values = aInput.getValues();

        std::string txId;
        std::vector<CPubKey> publicKeys;
        std::string path;
        CScript script;
        int nInput = -1;

        for (j = 0; j < keys.size(); j++) {
            UniValue val = values[j];
            if (keys[j] == "txid")
                txId = val.get_str();

            if (keys[j] == "vout")
                nInput = val.get_int();

            if (keys[j] == "satoshis")
                inTotal = val.get_int();

            if (keys[j] == "path")
                path = val.get_str();

            if (keys[j] == "publicKeys") {
                std::vector<UniValue> pubKeyValue = val.getValues();
                std::vector<std::string> keys;
                int k;
                for (k = 0; k < pubKeyValue.size(); k++) {
                    UniValue aPubKeyObj = pubKeyValue[k];
                    keys.push_back(aPubKeyObj.get_str());
                }
                std::sort(keys.begin(), keys.end());
                for (k = 0; k < keys.size(); k++) {
                    CPubKey vchPubKey(ParseHex(keys[k]));
                    publicKeys.push_back(vchPubKey);
                }
            }
        }
        uint256 aHash;
        aHash.SetHex(txId);
        script << OP_0 << OP_PUSHDATA1 << OP_VERIFY;
        script += GetScriptForMultisig(requiredSignatures, publicKeys);

        path.erase(0, 2); //remove m/ from path
        inputsScriptAndPath.push_back(std::make_pair(path, GetScriptForMultisig(requiredSignatures, publicKeys)));
        t.vin.insert(t.vin.begin(), CTxIn(aHash, nInput, script));
    }

    UniValue changeAddrObj = find_value(txProposal, "changeAddress");
    keys = changeAddrObj.getKeys();
    values = changeAddrObj.getValues();
    std::string changeAdr = "";
    for (i = 0; i < keys.size(); i++) {
        UniValue val = values[i];

        if (keys[i] == "address")
            changeAdr = val.get_str();
    }


    SelectParams(CBaseChainParams::TESTNET);
    CBitcoinAddress addr(toAddress);
    CBitcoinAddress addrC(changeAdr);
    CScript scriptPubKey = GetScriptForDestination(addr.Get());
    CScript scriptPubKeyC = GetScriptForDestination(addrC.Get());

    CTxOut txout(toAmount, scriptPubKey);
    t.vout.push_back(txout);

    CTxOut txoutC(inTotal - toAmount - fee, scriptPubKeyC);
    if (outputOrder[1] == 0)
        t.vout.insert(t.vout.begin(), txoutC);
    else
        t.vout.push_back(txoutC);

    int cnt = 0;
    for (const CTxIn& txIn : t.vin) {
        std::pair<std::string, CScript> scriptAndPath = inputsScriptAndPath[cnt];
        CScript aScript = scriptAndPath.second;
        uint256 hash = SignatureHash(aScript, t, 0, SIGHASH_ALL);
        vInputTxHashes.push_back(std::make_pair(scriptAndPath.first, hash));
        cnt++;
    }
    std::string hex = EncodeHexTx(t);
    SelectParams(CBaseChainParams::TESTNET);

    return hex;
}

int ecdsa_sig_to_der(const uint8_t* sig, uint8_t* der)
{
    int i;
    uint8_t* p = der, *len, *len1, *len2;
    *p = 0x30;
    p++; // sequence
    *p = 0x00;
    len = p;
    p++; // len(sequence)

    *p = 0x02;
    p++; // integer
    *p = 0x00;
    len1 = p;
    p++; // len(integer)

    // process R
    i = 0;
    while (sig[i] == 0 && i < 32) {
        i++;
    }                     // skip leading zeroes
    if (sig[i] >= 0x80) { // put zero in output if MSB set
        *p = 0x00;
        p++;
        *len1 = *len1 + 1;
    }
    while (i < 32) { // copy bytes to output
        *p = sig[i];
        p++;
        *len1 = *len1 + 1;
        i++;
    }

    *p = 0x02;
    p++; // integer
    *p = 0x00;
    len2 = p;
    p++; // len(integer)

    // process S
    i = 32;
    while (sig[i] == 0 && i < 64) {
        i++;
    }                     // skip leading zeroes
    if (sig[i] >= 0x80) { // put zero in output if MSB set
        *p = 0x00;
        p++;
        *len2 = *len2 + 1;
    }
    while (i < 64) { // copy bytes to output
        *p = sig[i];
        p++;
        *len2 = *len2 + 1;
        i++;
    }

    *len = *len1 + *len2 + 4;
    return *len + 2;
}

bool BitPayWalletClient::PostSignaturesForTxProposal(const UniValue& txProposal, const std::vector<std::string>& vHexSigs)
{
    //parse out the txpid
    UniValue pID = find_value(txProposal, "id");
    std::string txpID = "";
    if (pID.isStr())
        txpID = pID.get_str();

    UniValue signaturesRequest = UniValue(UniValue::VOBJ);
    UniValue sigs = UniValue(UniValue::VARR);
    for (const std::string& sSig : vHexSigs) {
        std::vector<unsigned char> data = ParseHex(sSig);
        unsigned char sig[74];
        int sizeN = ecdsa_sig_to_der(&data[0], sig);
        std::string sSigDER = HexStr(sig, sig + sizeN);
        sigs.push_back(sSigDER);
    }
    signaturesRequest.push_back(Pair("signatures", sigs));
    std::string response;
    long httpStatusCode = 0;
    SendRequest("post", "/v1/txproposals/" + txpID + "/signatures/", signaturesRequest.write(), response, httpStatusCode);
    if (httpStatusCode != 200)
        return false;

    return true;
}

bool BitPayWalletClient::BroadcastProposal(const UniValue& txProposal)
{
    std::string requestPubKey;
    if (!GetRequestPubKey(requestPubKey))
        return false;

    UniValue pID = find_value(txProposal, "id");
    std::string txpID = "";
    if (pID.isStr())
        txpID = pID.get_str();

    std::string response;
    long httpStatusCode = 0;
    SendRequest("post", "/v1/txproposals/" + txpID + "/broadcast/", "{}", response, httpStatusCode);
    printf("Response: %s\n", response.c_str());
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
    printf("signing message: %s\n", message.c_str());
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
        chunk = curl_slist_append(chunk, ("x-identity: " + GetCopayerId()).c_str()); //requestPubKey).c_str());
        chunk = curl_slist_append(chunk, ("x-signature: " + signature).c_str());
        chunk = curl_slist_append(chunk, "Content-Type: application/json");
        res = curl_easy_setopt(curl, CURLOPT_HTTPHEADER, chunk);
        curl_easy_setopt(curl, CURLOPT_URL, (baseURL + url).c_str());
        if (method == "post")
            curl_easy_setopt(curl, CURLOPT_POSTFIELDS, args.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &responseOut);

#ifdef DBB_ENABLE_DEBUG
        curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
#endif

        res = curl_easy_perform(curl);
        if (res != CURLE_OK) {
            fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
            error = true;
        } else {
            curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &httpcodeOut);
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
    FILE* writeFile = fopen((dataDir / "copay.dat").string().c_str(), "wb");
    if (writeFile) {
        CAutoFile copayDatFile(writeFile, SER_DISK, 1);
        copayDatFile << requestKey.GetPrivKey();


        CKeyingMaterial encoded;
        encoded.resize(76);

        CBitcoinExtPubKey b58PubkeyDecodeCheck(masterPubKey);
        printf("Save Master XPubKey: %s\n ", b58PubkeyDecodeCheck.ToString().c_str());

        masterPubKey.Encode(&encoded[0]);
        copayDatFile << encoded;
    }
    fclose(writeFile);
}

void BitPayWalletClient::LoadLocalData()
{
    boost::filesystem::path dataDir = GetDefaultDBBDataDir();
    boost::filesystem::create_directories(dataDir);
    FILE* fh = fopen((dataDir / "copay.dat").string().c_str(), "rb");
    if (fh) {
        CAutoFile copayDatFile(fh, SER_DISK, 1);
        if (!copayDatFile.IsNull()) {
            CPrivKey pkey;
            copayDatFile >> pkey;
            requestKey.SetPrivKey(pkey, true);

            CKeyingMaterial encoded;
            encoded.resize(74);
            copayDatFile >> encoded;

            masterPubKey.Decode(&encoded[0]);

            CBitcoinExtPubKey b58PubkeyDecodeCheck(masterPubKey);
            printf("Load Master XPubKey: %s\n", b58PubkeyDecodeCheck.ToString().c_str());
        }
        fclose(fh);
    }
}

void BitPayWalletClient::RemoveLocalData()
{
    boost::filesystem::path dataDir = GetDefaultDBBDataDir();
    boost::filesystem::remove(dataDir / "copay.dat");
}