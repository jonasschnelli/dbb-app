// Copyright (c) 2015 Jonas Schnelli
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef DBBAPP_UPDATE_H
#define DBBAPP_UPDATE_H

#include <QWidget>

#include "univalue.h"

class DBBUpdateManager : public QWidget
{
    Q_OBJECT

signals:
    //emitted when check-for-updates response is available
    void checkForUpdateResponseAvailable(const std::string&, long, bool);
    void updateButtonSetAvailable(bool);

public:
    DBBUpdateManager();
    //set the dynamic/runtime CA file for https requests
    void setCAFile(const std::string& ca_file);
    void setSocks5ProxyURL(const std::string& proxyUrl);
    
public slots:
    void checkForUpdateInBackground();
    void checkForUpdate(bool reportAlways = true);
    void parseCheckUpdateResponse(const std::string &response, long statusCode, bool reportAlways);

private:
    bool SendRequest(const std::string& method, const std::string& url, const std::string& args, std::string& responseOut, long& httpcodeOut);
    bool checkingForUpdates;
    std::string ca_file;

    std::string socks5ProxyURL;
};
#endif // DBBAPP_UPDATE_H
