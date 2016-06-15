/*

 The MIT License (MIT)

 Copyright (c) 2015 Jonas Schnelli

 Permission is hereby granted, free of charge, to any person obtaining
 a copy of this software and associated documentation files (the "Software"),
 to deal in the Software without restriction, including without limitation
 the rights to use, copy, modify, merge, publish, distribute, sublicense,
 and/or sell copies of the Software, and to permit persons to whom the
 Software is furnished to do so, subject to the following conditions:

 The above copyright notice and this permission notice shall be included
 in all copies or substantial portions of the Software.

 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES
 OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 OTHER DEALINGS IN THE SOFTWARE.

*/

#include "dbb_app.h"

#include <assert.h>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <iostream>
#include <mutex>
#include <queue>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <thread>

#include "dbb.h"
#include "dbb_util.h"

#include "univalue.h"
#include "libbitpay-wallet-client/bpwalletclient.h"

#include "hidapi/hidapi.h"

#ifdef WIN32
#include <signal.h>
#include "mingw/mingw.mutex.h"
#include "mingw/mingw.condition_variable.h"
#include "mingw/mingw.thread.h"
#else
#include <sys/signal.h>
#endif

#include "config/_dbb-config.h"

#include <btc/ecc.h>

#ifdef DBB_ENABLE_QT
#include <QApplication>
#include <QPushButton>

#include "qt/dbb_gui.h"
#if defined(DBB_QT_STATICPLUGIN)
#include <QtPlugin>
#if QT_VERSION < 0x050000
Q_IMPORT_PLUGIN(qcncodecs)
Q_IMPORT_PLUGIN(qjpcodecs)
Q_IMPORT_PLUGIN(qtwcodecs)
Q_IMPORT_PLUGIN(qkrcodecs)
Q_IMPORT_PLUGIN(qtaccessiblewidgets)
#else
#if QT_VERSION < 0x050400
Q_IMPORT_PLUGIN(AccessibleFactory)
#endif
#if defined(DBB_QT_QPA_PLATFORM_XCB)
Q_IMPORT_PLUGIN(QXcbIntegrationPlugin);
#elif defined(DBB_QT_QPA_PLATFORM_WINDOWS)
Q_IMPORT_PLUGIN(QWindowsIntegrationPlugin);
Q_IMPORT_PLUGIN(DSServicePlugin);
#elif defined(DBB_QT_QPA_PLATFORM_COCOA)
Q_IMPORT_PLUGIN(QCocoaIntegrationPlugin);
Q_IMPORT_PLUGIN(AVFServicePlugin);
#endif
#endif
#endif

extern void doubleSha256(char* string, unsigned char* hashOut);

static DBBDaemonGui* widget;
#endif

std::condition_variable queueCondVar;
std::mutex cs_queue;

//TODO: migrate tuple to a class
typedef std::tuple<std::string, std::string, std::function<void(const std::string&, dbb_cmd_execution_status_t status)> > t_cmdCB;
std::queue<t_cmdCB> cmdQueue;
std::atomic<bool> stopThread(false);
std::atomic<bool> notified(false);

std::atomic<bool> firmwareUpdateHID(false);

void setFirmwareUpdateHID(bool state)
{
    firmwareUpdateHID = state;
}

//executeCommand adds a command to the thread queue and notifies the tread to work down the queue
void executeCommand(const std::string& cmd, const std::string& password, std::function<void(const std::string&, dbb_cmd_execution_status_t status)> cmdFinished)
{
    std::unique_lock<std::mutex> lock(cs_queue);
    cmdQueue.push(t_cmdCB(cmd, password, cmdFinished));
    notified = true;
    queueCondVar.notify_one();
}

int main(int argc, char** argv)
{
    DBB::ParseParameters(argc, argv);

    //TODO: factor out thread
    std::thread cmdThread([&]() {
        //TODO, the locking is to broad at the moment
        //  during executing a command the queue is locked
        //  and therefore no new commands can be added
        //  copy the command and callback and release the lock
        //  would be a solution

        std::unique_lock<std::mutex> lock(cs_queue);
        while (!stopThread) {
            while (!notified) {  // loop to avoid spurious wakeups
                queueCondVar.wait(lock);
            }
            while (!cmdQueue.empty()) {
                std::string cmdOut;
                t_cmdCB cmdCB = cmdQueue.front();
                std::string cmd = std::get<0>(cmdCB);
                std::string password = std::get<1>(cmdCB);
                dbb_cmd_execution_status_t status = DBB_CMD_EXECUTION_STATUS_OK;

                if (!password.empty())
                {
                    std::string base64str;
                    std::string unencryptedJson;
                    try
                    {
                        DebugOut("sendcmd", "encrypt&send: %s\n", cmd.c_str());
                        DBB::encryptAndEncodeCommand(cmd, password, base64str);
                        if (!DBB::sendCommand(base64str, cmdOut))
                        {
                            DebugOut("sendcmd", "sending command failed\n");
                            status = DBB_CMD_EXECUTION_STATUS_ENCRYPTION_FAILED;
                        }
                        else
                            DBB::decryptAndDecodeCommand(cmdOut, password, unencryptedJson);
                    }
                    catch (const std::exception& ex) {
                        unencryptedJson = cmdOut;
                        DebugOut("sendcmd", "response decryption failed: %s\n", unencryptedJson.c_str());
                        status = DBB_CMD_EXECUTION_STATUS_ENCRYPTION_FAILED;
                    }

                    cmdOut = unencryptedJson;
                }
                else
                {
                    DebugOut("sendcmd", "send unencrypted: %s\n", cmd.c_str());
                    DBB::sendCommand(cmd, cmdOut);
                }
                std::get<2>(cmdCB)(cmdOut, status);
                cmdQueue.pop();
            }
            notified = false;
        }
    });

    //create a thread for the http handling
    std::thread usbCheckThread([&]() {
        while(1)
        {
            //check devices
            enum DBB::dbb_device_mode deviceType = DBB::deviceAvailable();

            if (!DBB::isConnectionOpen() || deviceType == DBB::DBB_DEVICE_NO_DEVICE || deviceType == DBB::DBB_DEVICE_UNKNOWN)
            {
                bool openSuccess = false;
                if (deviceType == DBB::DBB_DEVICE_MODE_BOOTLOADER)
                    openSuccess = DBB::openConnection(HID_BL_BUF_SIZE_W, HID_BL_BUF_SIZE_R);
                else
                    openSuccess = DBB::openConnection();

#ifdef DBB_ENABLE_QT
                //TODO, check if this requires locking
                if (widget)
                    widget->deviceStateHasChanged(openSuccess, deviceType);
#endif
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        }
    });

    btc_ecc_start();
    // Generate high-dpi pixmaps
#if defined(Q_OS_MAC)
    QApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);
#endif

    QString bitcoinURL;
    for (int i = 1; i < argc; i++)
    {
        QString arg(argv[i]);
        if (arg.startsWith("-"))
            continue;

        if (arg.startsWith("bitcoin:", Qt::CaseInsensitive)) // bitcoin: URI
            bitcoinURL = arg;
    }
    QApplication app(argc, argv);

    std::string dataDir = DBB::GetDefaultDBBDataDir();
    DBB::CreateDir(dataDir.c_str());
    DBB::OpenDebugLog();
    DBB::LogPrint("\n\n\n\nStarting DBB App %s - %s\n", DBB_PACKAGE_VERSION, VERSION);
    widget = new DBBDaemonGui(bitcoinURL);
    widget->show();
    //set style sheets
    app.exec();

    DBB::closeConnection(); //clean up HID
    btc_ecc_stop();
    exit(1);
}
