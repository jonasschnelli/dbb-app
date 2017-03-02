// Copyright (c) 2015 Jonas Schnelli
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "update.h"

#include <curl/curl.h>

#include <dbb_netthread.h>
#include <dbb_util.h>

#include <QDesktopServices>
#include <QDialog>
#include <QMessageBox>
#include <QString>
#include <QUrl>
#include <QWidget>


static size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp)
{
    ((std::string*)userp)->append((char*)contents, size * nmemb);
    return size * nmemb;
}

DBBUpdateManager::DBBUpdateManager() : QWidget(), checkingForUpdates(0)
{
    connect(this, SIGNAL(checkForUpdateResponseAvailable(const std::string&, long, bool)), this, SLOT(parseCheckUpdateResponse(const std::string&, long, bool)));
    ca_file = "";
    socks5ProxyURL.clear();
}

bool DBBUpdateManager::SendRequest(const std::string& method,
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

        if (socks5ProxyURL.size())
            curl_easy_setopt(curl, CURLOPT_PROXY, socks5ProxyURL.c_str());
        
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, 15L);

#if defined(__linux__) || defined(__unix__)
        //need to libcurl, load it once, set the CA path at runtime
        //we assume only linux needs CA fixing
        curl_easy_setopt(curl, CURLOPT_CAINFO, ca_file.c_str());
#endif

#ifdef DBB_ENABLE_NETDEBUG
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

void DBBUpdateManager::checkForUpdateInBackground()
{
    checkForUpdate(false);
}

void DBBUpdateManager::checkForUpdate(bool reportAlways)
{
    if (checkingForUpdates)
        return;

    DBBNetThread* thread = DBBNetThread::DetachThread();
    thread->currentThread = std::thread([this, thread, reportAlways]() {
        std::string response;
        long httpStatusCode;
        SendRequest("post", "https://digitalbitbox.com/desktop-app/update.json", "{\"version\":\""+std::string(DBB_PACKAGE_VERSION)+"\",\"target\":\"dbb-app\",\"key\":\"KhT9Lzb6o4EYLOVAqjXVWENt6rVKruFVUVJmtxkXKXG5eDw\"}", response, httpStatusCode);
        emit checkForUpdateResponseAvailable(response, httpStatusCode, reportAlways);
        thread->completed();
    });

    checkingForUpdates = true;
}

void DBBUpdateManager::parseCheckUpdateResponse(const std::string &response, long statuscode, bool reportAlways)
{
    checkingForUpdates = false;

    UniValue jsonOut;
    jsonOut.read(response);

    try {
        if (jsonOut.isObject())
        {
            UniValue subtext = find_value(jsonOut, "message");
            UniValue url = find_value(jsonOut, "url");
            if (subtext.isStr() && url.isStr())
            {
                if (url.get_str().compare("") == 0) {
                    if (reportAlways)
                        QMessageBox::information(this, tr(""), QString::fromStdString(subtext.get_str()), QMessageBox::Ok);
                    emit updateButtonSetAvailable(false);
                    return;
                }

                if (reportAlways) {
                    QMessageBox::StandardButton reply = QMessageBox::question(this, "", QString::fromStdString(subtext.get_str()), QMessageBox::Yes | QMessageBox::No);
                    if (reply == QMessageBox::Yes)
                    {
                        QString link = QString::fromStdString(url.get_str());
                        QDesktopServices::openUrl(QUrl(link));
                    }
                    emit updateButtonSetAvailable(false);
                    return;
                }
                else
                    emit updateButtonSetAvailable(true);
            }
        }
        if (reportAlways)
            QMessageBox::warning(this, tr(""), tr("Error while checking for updates."), QMessageBox::Ok);
    } catch (std::exception &e) {
        DBB::LogPrint("Error while reading update json\n", "");
    }
}

void DBBUpdateManager::setCAFile(const std::string& ca_file_in)
{
    ca_file = ca_file_in;
}

void DBBUpdateManager::setSocks5ProxyURL(const std::string& proxyUrl)
{
    socks5ProxyURL = proxyUrl;
}
