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

class DBBComServer
{
private:
    DBBNetThread* longPollThread;
public:
    DBBComServer();
    ~DBBComServer();

    std::string channelID;
    std::vector<unsigned char> encryptionKey;

    bool generateNewKey();
    void startLongPollThread();
    bool SendRequest(const std::string& method, const std::string& url, const std::string& args, std::string& responseOut, long& httpcodeOut);
    bool postNotification(const std::string& payload);

    const std::string getPairData();
    const std::string getAESKeyBase58();

    const std::string getChannelID();
    void setChannelID(const std::string& channelID);

    const std::vector<unsigned char> getEncryptionKey();
    void setEncryptionKey(const std::vector<unsigned char> encryptionKeyIn);

    // super efficient C like callbacks
    // whatout, they are calling back on the poll thread!
    void (*parseMessageCB)(DBBComServer*, const std::string&, void *);
    void *ctx;
    void setParseMessageCB(void (*fpIn)(DBBComServer*, const std::string&, void *), void *ctx);
};

#endif //DBBAPP_COMSERVER_H