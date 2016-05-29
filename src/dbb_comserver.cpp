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
#include "dbb.h"
#include "univalue.h"

#include <string.h>

static const char *aesKeyHMAC_Key = "DBBAesKey";

// add definition of two non public libbtc functions
extern "C" {
extern void ripemd160(const uint8_t* msg, uint32_t msg_len, uint8_t* hash);
extern void hmac_sha256(const uint8_t* key, const uint32_t keylen, const uint8_t* msg, const uint32_t msglen, uint8_t* hmac);
}

static size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp)
{
    ((std::string*)userp)->append((char*)contents, size * nmemb);
    return size * nmemb;
}

/* this is how the CURLOPT_XFERINFOFUNCTION callback works */
static int xferinfo(void *p,
                    curl_off_t dltotal, curl_off_t dlnow,
                    curl_off_t ultotal, curl_off_t ulnow)
{
    if (p)
    {
        DBBComServer *cs = (DBBComServer *)p;
        return cs->shouldCancelLongPoll();
    }

    return 0;
}

static int progress_cb(void *p,
                          double dltotal, double dlnow,
                          double ultotal, double ulnow)
{
    return xferinfo(p,
                    (curl_off_t)dltotal,
                    (curl_off_t)dlnow,
                    (curl_off_t)ultotal,
                    (curl_off_t)ulnow);
}

DBBComServer::DBBComServer(const std::string& comServerURLIn) : longPollThread(0), comServerURL(comServerURLIn)
{
    channelID.clear();
    parseMessageCB = nullptr;
    nSequence = 0;
    mobileAppConnected = false;
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
    hash160[0] = CHANNEL_ID_BASE58_PREFIX;
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
        curl_easy_setopt(curl, CURLOPT_PROGRESSFUNCTION, progress_cb);
        curl_easy_setopt(curl, CURLOPT_PROGRESSDATA, this);
#if LIBCURL_VERSION_NUM >= 0x072000
        /* xferinfo was introduced in 7.32.0, no earlier libcurl versions will
         compile as they won't have the symbols around.

         If built with a newer libcurl, but running with an older libcurl:
         curl_easy_setopt() will fail in run-time trying to set the new
         callback, making the older callback get used.

         New libcurls will prefer the new callback and instead use that one even
         if both callbacks are set. */

        curl_easy_setopt(curl, CURLOPT_XFERINFOFUNCTION, xferinfo);
        /* pass the struct pointer into the xferinfo function, note that this is
         an alias to CURLOPT_PROGRESSDATA */
        curl_easy_setopt(curl, CURLOPT_XFERINFODATA, this);
#endif
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
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, 15L);
        curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0L);
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);

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

bool DBBComServer::shouldCancelLongPoll()
{
    // detects if a long poll needs to be cancled
    // because user have switched the channel
    std::unique_lock<std::mutex> lock(cs_com);
    return (currentLongPollChannelID != channelID || currentLongPollURL != comServerURL);
}

void DBBComServer::startLongPollThread()
{
    std::unique_lock<std::mutex> lock(cs_com);

    if (longPollThread)
        return;

    longPollThread = DBBNetThread::DetachThread();
    longPollThread->currentThread = std::thread([this]() {
        std::string response;
        long httpStatusCode;
        long sequence = 0;
        int errorCounts = 0;
        UniValue jsonOut;

        while(1)
        {
            response = "";
            httpStatusCode = 400;
            {
                // we store the channel ID to detect channelID changes during long poll 
                std::unique_lock<std::mutex> lock(cs_com);
                currentLongPollChannelID = channelID;
                currentLongPollURL = comServerURL;
            }
            SendRequest("post", currentLongPollURL, "c=gd&uuid="+currentLongPollChannelID+"&dt=0&s="+std::to_string(sequence), response, httpStatusCode);
            sequence++;

            if (httpStatusCode >= 300)
            {
                errorCounts++;
                if (errorCounts > 5)
                {
                    DBB::LogPrintDebug("Error, can't connect to the smart verification server");
                    // wait 10 seconds before the next try
                    std::this_thread::sleep_for(std::chrono::milliseconds(10000));
                }
                else
                    std::this_thread::sleep_for(std::chrono::milliseconds(2000));
            }
            else
                errorCounts = 0;

            // ignore the response if the channel has been switched (re-pairing)
            {
                std::unique_lock<std::mutex> lock(cs_com);
                if (currentLongPollChannelID != channelID || currentLongPollURL != comServerURL)
                    continue;
            }

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
                            std::string plaintextPayload;
                            std::string keyS(encryptionKey.begin(), encryptionKey.end());
                            if (DBB::decryptAndDecodeCommand(payload.get_str(), keyS, plaintextPayload, false))
                            {
                                std::unique_lock<std::mutex> lock(cs_com);
                                if (parseMessageCB)
                                    parseMessageCB(this, plaintextPayload, ctx);
                            }

                            // mem-cleanse the key
                            std::fill(keyS.begin(), keyS.end(), 0);
                            keyS.clear();
                        }
                    }
                }
            }
        }
        std::unique_lock<std::mutex> lock(cs_com);
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

        // encrypt the payload
        std::string encryptedPayload;
        std::string keyS(encryptionKey.begin(), encryptionKey.end());
        DBB::encryptAndEncodeCommand(payload, keyS, encryptedPayload, false);
        // mem-cleanse the key
        std::fill(keyS.begin(), keyS.end(), 0);
        keyS.clear();

        // send the payload
        SendRequest("post", comServerURL, "c=data&s="+std::to_string(nSequence)+"&uuid="+channelID+"&dt=0&pl="+encryptedPayload, response, httpStatusCode);
        nSequence++; // increase the sequence number

        // ignore the response for now
        postThread->completed();
    });

    return true;
}

const std::string DBBComServer::getPairData()
{
    std::string channelData = "{\"id\":\""+getChannelID()+"\",\"key\":\""+getAESKeyBase58()+"\"}";
    return channelData;
}

const std::string DBBComServer::getAESKeyBase58()
{
    std::string aesKeyBase58;
    aesKeyBase58.resize(100);
    uint8_t hash[33];
    hash[0] = AES_KEY_BASE57_PREFIX;
    assert(encryptionKey.size() > 0);

    std::string base64dec = base64_encode(&encryptionKey[0], encryptionKey.size());
    return base64dec;

    hmac_sha256((const uint8_t *)aesKeyHMAC_Key, strlen(aesKeyHMAC_Key), &encryptionKey[0], encryptionKey.size(), hash);

    btc_hash(&encryptionKey[0], encryptionKey.size(), hash);
    int sizeOut = btc_base58_encode_check(hash, 33, &aesKeyBase58[0], aesKeyBase58.size());
    aesKeyBase58.resize(sizeOut-1);
    return aesKeyBase58;
}

const std::string DBBComServer::getChannelID()
{
    std::unique_lock<std::mutex> lock(cs_com);
    return channelID;
}

void DBBComServer::setChannelID(const std::string& channelIDIn)
{
    std::unique_lock<std::mutex> lock(cs_com);
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

void DBBComServer::setCAFile(const std::string& ca_fileIn)
{
    ca_file = ca_fileIn;
}
