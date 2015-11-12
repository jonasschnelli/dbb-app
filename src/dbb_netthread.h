// Copyright (c) 2015 Jonas Schnelli
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef DBBAPP_NETTHREAD_H
#define DBBAPP_NETTHREAD_H

#include <functional>
#include <mutex>
#include <thread>
#include <vector>

#include "univalue.h"

class DBBNetThread
{
private:
    bool finished;
    std::time_t starttime;

public:
    DBBNetThread();
    ~DBBNetThread();
    void completed();
    bool hasCompleted();
    std::thread currentThread;

    static DBBNetThread* DetachThread();
    static void CleanupThreads();
    static std::vector<DBBNetThread *> netThreads;
    static std::mutex cs_netThreads;
};
#endif