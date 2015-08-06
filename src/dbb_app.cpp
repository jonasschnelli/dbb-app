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
#include "util.h"

#include "univalue.h"
#include "libbitpay-wallet-client/bpwalletclient.h"

#include "hidapi/hidapi.h"
#include "openssl/sha.h"
#include <openssl/err.h>
#include <openssl/rand.h>

#include <event2/event.h>
#include <event2/http.h>
#include <event2/buffer.h>
#include <event2/util.h>
#include <event2/keyvalq_struct.h>
#include <sys/signal.h>

#ifdef ENABLE_QT
#include <QApplication>
#include <QPushButton>

#include "qt/dbbgui.h"

extern void doubleSha256(char* string, unsigned char* hashOut);

static DBBDaemonGui* widget;
#endif

std::condition_variable queueCondVar;
std::mutex cs_queue;

//TODO: migrate tuple to a class
typedef std::tuple<std::string, std::string, std::function<void(const std::string&)> > t_cmdCB;
std::queue<t_cmdCB> cmdQueue;
std::atomic<bool> stopThread;
std::atomic<bool> notified;

//executeCommand adds a command to the thread queue and notifies the tread to work down the queue
void executeCommand(const std::string& cmd, const std::string& password, std::function<void(const std::string&)> cmdFinished)
{
    std::unique_lock<std::mutex> lock(cs_queue);
    cmdQueue.push(t_cmdCB(cmd, password, cmdFinished));
    notified = true;
    queueCondVar.notify_one();
}

//simple function for the LED blick command
static void led_blink(struct evhttp_request* req, void* arg)
{
    printf("Received a request for %s\nDispatching dbb command\n", evhttp_request_get_uri(req));

    //dispatch command
    executeCommand("{\"led\" : \"toggle\"}", "0000", [](const std::string& cmdOut) {
    });

    //form a response, mind, no cmd result is available at this point, at the moment we don't block the http response thread
    struct evbuffer* out = evbuffer_new();
    evbuffer_add_printf(out, "Command dispatched\n");
    evhttp_send_reply(req, 200, "OK", out);
}

char uri_root[512];
int main(int argc, char** argv)
{
    struct event_base* base;
    struct evhttp* http;
    struct evhttp_bound_socket* handle;

    unsigned short port = 15520;

    if (signal(SIGPIPE, SIG_IGN) == SIG_ERR)
        return (1);

    base = event_base_new();
    if (!base) {
        fprintf(stderr, "Couldn't create an event_base: exiting\n");
        return 1;
    }

    http = evhttp_new(base);
    evhttp_set_cb(http, "/led/blink", led_blink, NULL);
    handle = evhttp_bind_socket_with_handle(http, "0.0.0.0", port);

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

                if (!password.empty())
                {
                    std::string base64str;
                    std::string unencryptedJson;
                    try
                    {
                        DBB::encryptAndEncodeCommand(cmd, password, base64str);
                        if (!DBB::sendCommand(base64str, cmdOut))
                            unencryptedJson = "sending command failed";
                        else
                            DBB::decryptAndDecodeCommand(cmdOut, password, unencryptedJson);
                    }
                    catch (const std::exception& ex) {
                        unencryptedJson = "response decryption failed: "+cmdOut;
                    }

                    cmdOut = unencryptedJson;
                }
                else
                {
                    DBB::sendCommand(cmd, cmdOut);
                }
                std::get<2>(cmdCB)(cmdOut);
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
            if (!DBB::isConnectionOpen())
            {
                printf("no connection\n");
                if (DBB::openConnection())
                {
#ifdef ENABLE_QT
                //TODO, check if this requires locking
                if (widget)
                    widget->deviceStateHasChanged(true);
#endif
                }
                else
                {
#ifdef ENABLE_QT
                if (widget)
                    widget->deviceStateHasChanged(false);
#endif
                }
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        }
    });
    
    
    ECC_Start();
    
    BitPayWalletClient client;
    CKey aKey = client.GetNewKey();
    assert(aKey.VerifyPubKey(aKey.GetPubKey()) == 1);

    ECC_Stop();
    
    // unsigned char vchPub[65];
    // int clen = 65;
    // secp256k1_pubkey_t pubkey;
    // assert(secp256k1_ec_seckey_verify(secp256k1_context, vch) == 1);
    // int ret = secp256k1_ec_pubkey_create(secp256k1_context, &pubkey, vch);
    // unsigned char pubkeyc[65];
    // int pubkeyclen = 65;
    // secp256k1_ec_pubkey_serialize(secp256k1_context, pubkeyc, &pubkeyclen, &pubkey, true);
    
    // std::string hexStr = HexStr(pubkeyc, pubkeyc+pubkeyclen, false);
    //
    // secp256k1_pubkey_t pubkeyNew;
    // std::vector<unsigned char> vPubKey = ParseHex(hexStr);
    // int suc = secp256k1_ec_pubkey_parse(secp256k1_context, &pubkeyNew, &vPubKey[0], vPubKey.size());
    

    
    //var message = [method.toLowerCase(), url, JSON.stringify(args)].join('|');
    // std::string requestString = "get|/v1/wallets/?r=62416|{}";
    // unsigned char hashD[32];
    // doubleSha256((char*)requestString.c_str(), hashD);
    // secp256k1_ecdsa_signature_t signature;
    // suc = secp256k1_ecdsa_sign(secp256k1_context, hashD, &signature, vch, NULL, NULL);
    //
    // unsigned char sig[74];
    // int siglen = 74;
    // suc = secp256k1_ecdsa_signature_serialize_der(secp256k1_context, sig, &siglen, &signature);
    // std::string hexStrDER = HexStr(sig, sig+siglen, false);
    //
    //
    // std::string header = "\n\ncurl --header \"x-identity: "+hexStr+"\" --header \"x-signature: "+hexStrDER+"\"  -v https://bws.bitpay.com/bws/api/v1/wallets/\n\n";
    // printf("header: %s", header.c_str());
    
#ifdef ENABLE_QT
#if QT_VERSION > 0x050100
    // Generate high-dpi pixmaps
    QApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);
#endif

    //create a thread for the http handling
    std::thread httpThread([&]() {
        event_base_dispatch(base);
    });
    
    QApplication app(argc, argv);

    widget = new DBBDaemonGui(0);
    widget->show();
    app.exec();
#else
    //directly start libevents main run loop
    event_base_dispatch(base);
#endif

    exit(1);
}
