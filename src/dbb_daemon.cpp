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
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>

#include <string>

#include "dbb.h"
#include "util.h"

#include "univalue.h"
#include "hidapi/hidapi.h"
#include "openssl/sha.h"

#include <event2/event.h>
#include <event2/http.h>
#include <event2/buffer.h>
#include <event2/util.h>
#include <event2/keyvalq_struct.h>
#include <sys/signal.h>

#include <thread> //c++11
#include <queue>
#include <mutex>
#include <condition_variable>
#include <atomic>

#ifdef ENABLE_QT
#include <QApplication>
#include <QPushButton>

#include "qt/daemongui.h"
#endif


std::condition_variable queueCondVar;
std::mutex cs_queue;

typedef std::tuple<std::string, std::string, std::function<void(const std::string&)> > t_cmdCB;
std::queue<t_cmdCB> cmdQueue;
std::atomic<bool> stopThread;
std::atomic<bool> notified;

//executeCommand adds a command to the thread queue and notifies the tread to work down the queue
void executeCommand(const std::string &cmd, const std::string &password, std::function<void(const std::string&)> cmdFinished)
{
    std::unique_lock<std::mutex> lock(cs_queue);
    cmdQueue.push(t_cmdCB(cmd, password, cmdFinished));
    notified = true;
    queueCondVar.notify_one();
}    

//simple function for the LED blick command
static void led_blink(struct evhttp_request *req, void *arg)
{
    printf("Received a request for %s\nDispatching dbb command\n", evhttp_request_get_uri(req));
    
    //dispatch command
    executeCommand("{\"led\" : \"toggle\"}", "0000", [](const std::string &cmdOut)
        {
        });
    
    //form a response, mind, no cmd result is available at this point, at the moment we don't block the http response thread
    struct evbuffer *out = evbuffer_new();
    evbuffer_add_printf(out, "Command dispatched\n");
    evhttp_send_reply(req, 200, "OK", out);
}

char uri_root[512];
int main(int argc, char **argv)
{
	struct event_base *base;
	struct evhttp *http;
	struct evhttp_bound_socket *handle;

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
    
    //create a thread for the http handling
    std::thread httpThread([&]() {
        event_base_dispatch(base);
    });

    //TODO: factor out thread 
    std::thread cmdThread([&]() {
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
                        DBB::sendCommand(base64str, cmdOut);
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
    
#ifdef ENABLE_QT
#if QT_VERSION > 0x050100
    // Generate high-dpi pixmaps
    QApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);
#endif

    QApplication app (argc, argv);

    DBBDaemonGui *widget = new DBBDaemonGui(0);
    widget->show();
    app.exec();
#endif
    
	exit(1);
}
