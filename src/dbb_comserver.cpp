// Copyright (c) 2015 Jonas Schnelli
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "dbb_comserver.h"

#include <curl/curl.h>

#include <btc/base58.h>
#include <btc/ecc_key.h>
#include <btc/hash.h>

#include "libdbb/crypto.h"

#include "dbb_util.h"
#include "univalue.h"

extern "C" {
extern void ripemd160(const uint8_t* msg, uint32_t msg_len, uint8_t* hash);
}

static size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp)
{
    ((std::string*)userp)->append((char*)contents, size * nmemb);
    return size * nmemb;
}

DBBComServer::DBBComServer()
{
    channelID.clear();
    parseMessageCB = nullptr;
    nSequence = 0;
}

DBBComServer::~DBBComServer()
{

}

bool DBBComServer::generateNewKey()
{
    // generate new private key
    btc_key key;
    btc_privkey_init(&key);
    btc_privkey_gen(&key);
    assert(btc_privkey_is_valid(&key) == 1);

    // derive pubkey
    btc_pubkey pubkey;
    btc_pubkey_init(&pubkey);
    btc_pubkey_from_key(&key, &pubkey);
    assert(btc_pubkey_is_valid(&pubkey) == 1);

    // remove the current enc key
    encryptionKey.clear();

    // copy over the privatekey and clean libbtc privkey
    std::copy(key.privkey,key.privkey+BTC_ECKEY_PKEY_LENGTH,std::back_inserter(encryptionKey));
    btc_privkey_cleanse(&key);

    // generate hash160(hash(pubkey))
    // create base58c string with 0x91 as base58 identifier
    size_t len = 67;
    uint8_t hashout[32];
    uint8_t hash160[21];
    hash160[0] = 0x91;
    btc_hash_sngl_sha256(pubkey.pubkey, BTC_ECKEY_COMPRESSED_LENGTH, hashout);
    ripemd160(hashout, 32, hash160+1);

    // make enought space for the base58c channel ID
    channelID.resize(100);
    int sizeOut = btc_base58_encode_check(hash160, 21, &channelID[0], channelID.size());
    channelID.resize(sizeOut-1);
    return true;
}

bool DBBComServer::SendRequest(const std::string& method,
                               const std::string& url,
                               const std::string& args,
                               std::string& responseOut,
                               long& httpcodeOut)
{
    CURL* curl;
    CURLcode res;

    bool success = false;

    curl_global_init(CURL_GLOBAL_ALL);
    curl = curl_easy_init();
    if (curl) {
        struct curl_slist* chunk = NULL;
        chunk = curl_slist_append(chunk, "Content-Type: text/plain");
        res = curl_easy_setopt(curl, CURLOPT_HTTPHEADER, chunk);
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        if (method == "post")
        {
            curl_easy_setopt(curl, CURLOPT_POSTFIELDS, args.c_str());
            curl_easy_setopt(curl, CURLOPT_POST, 1L);
        }

        if (method == "delete") {
            curl_easy_setopt(curl, CURLOPT_POSTFIELDS, args.c_str());
            curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "DELETE");
        }

        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &responseOut);
        curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L);
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, 15L);

#if defined(__linux__) || defined(__unix__)
        //need to libcurl, load it once, set the CA path at runtime
        //we assume only linux needs CA fixing
        curl_easy_setopt(curl, CURLOPT_CAINFO, ca_file.c_str());
#endif

#ifdef DBB_ENABLE_DEBUG
        curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
#endif

        res = curl_easy_perform(curl);
        if (res != CURLE_OK) {
            DBB::LogPrintDebug("curl_easy_perform() failed "+ ( curl_easy_strerror(res) ? std::string(curl_easy_strerror(res)) : ""), "");
            success = false;
        } else {
            curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &httpcodeOut);
            success = true;
        }

        curl_slist_free_all(chunk);
        curl_easy_cleanup(curl);
    }
    curl_global_cleanup();

    DBB::LogPrintDebug("response: "+responseOut, "");
    return success;
};

void DBBComServer::startLongPollThread()
{
    longPollThread = DBBNetThread::DetachThread();
    longPollThread->currentThread = std::thread([this]() {
        std::string response;
        long httpStatusCode;
        long sequence = 0;
        UniValue jsonOut;

        while(1)
        {
            response = "";
            httpStatusCode = 0;
            SendRequest("post", "https://bitcoin.jonasschnelli.ch/dbb/server.php", "c=gd&uuid="+channelID+"&dt=0&s="+std::to_string(sequence), response, httpStatusCode);
            sequence++;

            jsonOut.read(response);
            if (jsonOut.isObject())
            {
                UniValue data = find_value(jsonOut, "data");
                if (data.isArray())
                {
                    for (const UniValue& element : data.getValues())
                    {
                        UniValue payload = find_value(element, "payload");
                        if (payload.isStr())
                        {
                            std::string base64dec = base64_decode(payload.get_str());
                            printf("payload: %s\n", base64dec.c_str());

                            if (parseMessageCB)
                                parseMessageCB(this, base64dec, ctx);
                        }
                    }
                }
            }
        }
        longPollThread->completed();
    });
}

bool DBBComServer::postNotification(const std::string& payload)
{
    if (channelID.empty())
        return false;

    DBBNetThread *postThread = DBBNetThread::DetachThread();
    postThread->currentThread = std::thread([this, postThread, payload]() {
        std::string response;
        long httpStatusCode;
        UniValue jsonOut;

        response = "";
        httpStatusCode = 0;
        SendRequest("post", "https://bitcoin.jonasschnelli.ch/dbb/server.php", "c=data&s="+std::to_string(nSequence)+"&uuid="+channelID+"&dt=0&pl="+base64_encode((const unsigned char *)&payload[0], payload.size()), response, httpStatusCode);
        nSequence++; // increase the sequence number

        jsonOut.read(response);
        if (jsonOut.isObject())
        {
            UniValue data = find_value(jsonOut, "data");
            if (data.isArray())
            {
                for (const UniValue& element : data.getValues())
                {
                    UniValue payload = find_value(element, "payload");
                    if (payload.isStr())
                    {
                        std::string base64dec = base64_decode(payload.get_str());
                        printf("payload: %s\n", base64dec.c_str());
                    }
                }
            }
        }

        postThread->completed();
    });

    return true;
}

const std::string DBBComServer::getPairData()
{
    std::string channelData = getChannelID()+"-"+getAESKeyBase58();
    return channelData;
}

const std::string DBBComServer::getAESKeyBase58()
{
    std::string aesKeyBase58;
    aesKeyBase58.resize(100);
    uint8_t hash[33];
    hash[0] = 180;
    assert(encryptionKey.size() > 0);
    btc_hash(&encryptionKey[0], encryptionKey.size(), hash);
    int sizeOut = btc_base58_encode_check(hash, 33, &aesKeyBase58[0], aesKeyBase58.size());
    aesKeyBase58.resize(sizeOut-1);
    return aesKeyBase58;
}

const std::string DBBComServer::getChannelID()
{
    return channelID;
}

void DBBComServer::setChannelID(const std::string& channelIDIn)
{
    channelID = channelIDIn;
}

const std::vector<unsigned char> DBBComServer::getEncryptionKey()
{
    return encryptionKey;
}

void DBBComServer::setEncryptionKey(const std::vector<unsigned char> encryptionKeyIn)
{
    encryptionKey = encryptionKeyIn;
}

void DBBComServer::setParseMessageCB(void (*fpIn)(DBBComServer*, const std::string&, void*), void *ctxIn)
{
    parseMessageCB = fpIn;
    ctx = ctxIn;
}
