// Copyright (c) 2015 Jonas Schnelli
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef DBBAPP_COMSERVER_H
#define DBBAPP_COMSERVER_H

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <functional>
#include <mutex>
#include <thread>
#include <vector>
#include <string>
#include "dbb_netthread.h"

#ifdef WIN32
#include <windows.h>
#include "mingw/mingw.mutex.h"
#include "mingw/mingw.condition_variable.h"
#include "mingw/mingw.thread.h"
#endif

// this class manages push notification from and to the smart verification device
// the push messages will be sent to a proxy server script.
// receiving push messages is done with http long polling.


#define CHANNEL_ID_BASE58_PREFIX 0x91
#define AES_KEY_BASE57_PREFIX 0x56

/*
   symetric key and channel ID derivation

   channelID = base58check(RIPEMD160(SHA256(pubkey)))
   aeskey = sha256_hmac(key="DBBAesKey", encryption_ec_private_key)
*/
class DBBComServer
{
private:
    DBBNetThread* longPollThread; //!< the thread to handle the long polling (will run endless)
    std::string comServerURL; //!< the url to call
    std::string ca_file; //<!ca_file to use
    std::string socks5ProxyURL; //<!socks5 URL or empty for no proxy
    std::atomic<bool> shouldCancel;

    /* send a synchronous http request */
    bool SendRequest(const std::string& method, const std::string& url, const std::string& args, std::string& responseOut, long& httpcodeOut);

public:
    DBBComServer(const std::string& comServerURL);
    ~DBBComServer();

    std::mutex cs_com;
    std::string channelID; //!< channel ID (hash of the encryption pubkey)
    std::string currentLongPollChannelID; //!< channel ID that is currently in polling
    std::string currentLongPollURL; //!< comserver URL that is currently in polling

    /* encryption key (32byte ECC key), will be used to derive the AES key and the channel ID */
    std::vector<unsigned char> encryptionKey;

    // super efficient C callbacks
    // watch out! they are calling back on the poll thread!
    void (*parseMessageCB)(DBBComServer*, const std::string&, void *);
    void *ctx;

    long nSequence; //!< current sequence number for outgoing push messages
    
    /* updated depending on response to 'ping' command */
    bool mobileAppConnected;

    /* change/set the smart verification server URL */
    void setURL(const std::string& newUrl) { comServerURL = newUrl; }

    /* generates a new encryption key => new AES key, new channel ID */
    bool generateNewKey();

    /* starts the longPollThread, needs only be done once */
    void startLongPollThread();

    /* can be called during long poll idle to see if the channelID poll still makes sense */
    bool shouldCancelLongPoll();

    /* send a push notification to the server */
    bool postNotification(const std::string& payload);

    /* response the pair data (for QR Code generation)
       pair data = base58(channelID) & base58(AES_KEY)
     */
    const std::string getPairData();

    /* get the base58check encoded aes key */
    const std::string getAESKeyBase58();

    /* get the channelID */
    const std::string getChannelID();

    /* set the channelID, will terminate/reinitiate the long poll request */
    void setChannelID(const std::string& channelID);

    /* get the raw encryption key (for the persistance store) */
    const std::vector<unsigned char> getEncryptionKey();

    /* set the raw encryption key (32byte ec private key)*/
    void setEncryptionKey(const std::vector<unsigned char> encryptionKeyIn);

    /* set the parse message callback
       will be called whenever a new JSON message is available */
    void setParseMessageCB(void (*fpIn)(DBBComServer*, const std::string&, void *), void *ctx);

    void setCAFile(const std::string& ca_file);
    void setSocks5ProxyURL(const std::string& socksProxy);
};

#endif //DBBAPP_COMSERVER_H
