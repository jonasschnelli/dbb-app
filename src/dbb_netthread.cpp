// Copyright (c) 2015 Jonas Schnelli
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "dbb_netthread.h"

std::vector<DBBNetThread*> DBBNetThread::netThreads;
std::mutex DBBNetThread::cs_netThreads;

void DBBNetThread::completed()
{
    finished = true;
}

bool DBBNetThread::hasCompleted()
{
    return finished;
}

DBBNetThread::DBBNetThread()
{
    finished = false;
    starttime = std::time(0);
}

DBBNetThread* DBBNetThread::DetachThread()
{
    CleanupThreads();

    DBBNetThread* thread = new DBBNetThread();
    {
        std::unique_lock<std::mutex> lock(cs_netThreads);
        netThreads.push_back(thread);
    }
    return thread;
}

void DBBNetThread::CleanupThreads()
{
    std::unique_lock<std::mutex> lock(cs_netThreads);

    std::vector<DBBNetThread*>::iterator it = netThreads.begin();
    while (it != netThreads.end()) {
        DBBNetThread* netThread = (*it);
        if (netThread->hasCompleted()) {
            if (netThread->currentThread.joinable())
                netThread->currentThread.join();

            delete netThread;
            it = netThreads.erase(it);
        } else {
            ++it;
        }
    }
}

DBBNetThread::~DBBNetThread()
{
}