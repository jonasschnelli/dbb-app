// Copyright (c) 2016 Jonas Schnelli
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef DBB_CONFIGDATA_H
#define DBB_CONFIGDATA_H

#ifndef _SRC_CONFIG__DBB_CONFIG_H
#include "config/_dbb-config.h"
#endif

#include <univalue.h>
#include "libdbb/crypto.h"

#include <string>
#include <fstream>

static const char *bwsDefaultBackendURL = "https://bws.bitpay.com/bws/api";
static const char *defaultDBBProxyURL = "https://bws.digitalbitbox.com/bws/api";
static const char *defaultTorProxyURL = "socks5://localhost:9050";
static const char *comServerDefaultURL = "https://digitalbitbox.com/smartverification/index.php";

static const char *kVERSION = "version";
static const char *kCOM_CHANNEL_ID = "comserverchannelid";
static const char *kENC_PKEY = "encryptionprivkey";
static const char *kBWS_URL = "bws_url";
static const char *kCOMSERVER_URL = "comsrv_url";
static const char *kSOCKS5_PROXY = "socks5_url";
static const char *kUSE_DEFAULT_PROXY = "use_default_proxy";
static const char *kDBB_PROXY = "dbb_proxy";

namespace DBB
{

// simple model/controller class for persistance use settings
// uses JSON/file as persistance store
class DBBConfigdata {
private:
    static const int CURRENT_VERSION=1;
    std::string filename;

    int32_t version;
    std::string comServerURL;
    std::string bwsBackendURL;
    std::string comServerChannelID;
    std::vector<unsigned char> encryptionKey;
    std::string socks5ProxyURL;
    bool dbbProxy;
    bool useDefaultProxy;

public:
    DBBConfigdata(const std::string& filenameIn)
    {
        bwsBackendURL = std::string(bwsDefaultBackendURL);
        comServerURL = std::string(comServerDefaultURL);
        filename = filenameIn;
        version = CURRENT_VERSION;
        encryptionKey.clear();
        encryptionKey.resize(32);
        memset(&encryptionKey[0], 0, 32);
        dbbProxy = false;
        useDefaultProxy = false;
    }

    std::string getComServerURL() { return comServerURL; }
    void setComServerURL(const std::string& newURL) { comServerURL = newURL; }

    std::string getComServerChannelID() { return comServerChannelID; }
    void setComServerChannelID(const std::string& newID) { comServerChannelID = newID; }

    std::vector<unsigned char> getComServerEncryptionKey() { return encryptionKey; }
    void setComServerEncryptionKey(const std::vector<unsigned char>& newKey) { encryptionKey = newKey; }

    std::string getBWSBackendURLInternal()
    {
        return bwsBackendURL;
    }

    std::string getBWSBackendURL()
    {
        if (dbbProxy && ( bwsBackendURL == bwsDefaultBackendURL ))
            return defaultDBBProxyURL;

        return bwsBackendURL;
    }
    void setBWSBackendURL(const std::string& newURL) { bwsBackendURL = newURL; }

    std::string getSocks5ProxyURLInternal() {
        return socks5ProxyURL;
    }

    std::string getSocks5ProxyURL() {
        if (!useDefaultProxy)
            return std::string();

        if (socks5ProxyURL.size() > 0)
            return socks5ProxyURL;

        return defaultTorProxyURL;
    }
    void setSocks5ProxyURL(const std::string& in_socks5ProxyURL) { socks5ProxyURL = in_socks5ProxyURL; }

    bool getDBBProxy() { return dbbProxy; }
    void setDBBProxy(bool newState) { dbbProxy = newState; }

    bool getUseDefaultProxy() { return useDefaultProxy; }
    void setUseDefaultProxy(bool newState) { useDefaultProxy = newState; }

    std::string getDefaultBWSULR() { return bwsDefaultBackendURL; }
    std::string getDefaultComServerURL() { return comServerDefaultURL; }


    bool write()
    {
        UniValue objData(UniValue::VOBJ);
        objData.pushKV(kVERSION, version);
        objData.pushKV(kENC_PKEY, base64_encode(&encryptionKey[0], 32));
        objData.pushKV(kCOM_CHANNEL_ID, comServerChannelID);

        if (bwsBackendURL != bwsDefaultBackendURL)
            objData.pushKV(kBWS_URL, bwsBackendURL);

        if (comServerURL != comServerDefaultURL)
            objData.pushKV(kCOMSERVER_URL, comServerURL);

        if (!socks5ProxyURL.empty())
            objData.pushKV(kSOCKS5_PROXY, socks5ProxyURL);


        UniValue dbbProxyU(UniValue::VBOOL);
        dbbProxyU.setBool(dbbProxy);
        objData.pushKV(kDBB_PROXY, dbbProxyU);

        UniValue defaultProxyU(UniValue::VBOOL);
        defaultProxyU.setBool(useDefaultProxy);
        objData.pushKV(kUSE_DEFAULT_PROXY, defaultProxyU);

        std::string json = objData.write();
        FILE* writeFile = fopen(filename.c_str(), "w");
        if (writeFile) {
            fwrite(&json[0], 1, json.size(), writeFile);
            fclose(writeFile);
        }
        return true;
    }

    bool read()
    {
        FILE* readFile = fopen(filename.c_str(), "r");
        std::string json;
        if (readFile) {
            fseek(readFile,0,SEEK_END);
            int size = ftell(readFile);
            json.resize(size);
            fseek(readFile,0,SEEK_SET);
            fread(&json[0], 1, size, readFile);
            fclose(readFile);
        }

        UniValue objData(UniValue::VOBJ);
        objData.read(json);
        if (objData.isObject())
        {
            UniValue versionU = find_value(objData, kVERSION);
            if (versionU.isNum())
                version = versionU.get_int();

            UniValue comServerChannelIDU = find_value(objData, kCOM_CHANNEL_ID);
            if (comServerChannelIDU.isStr())
                comServerChannelID = comServerChannelIDU.get_str();

            UniValue comServerURLU = find_value(objData, kCOMSERVER_URL);
            if (comServerURLU.isStr())
                comServerURL = comServerURLU.get_str();

            UniValue bwsBackendURLU = find_value(objData, kBWS_URL);
            if (bwsBackendURLU.isStr())
                bwsBackendURL = bwsBackendURLU.get_str();

            UniValue encryptionKeyU = find_value(objData, kENC_PKEY);
            if (encryptionKeyU.isStr())
            {
                std::string encryptionKeyS = base64_decode(encryptionKeyU.get_str());
                encryptionKey.clear();
                std::copy(encryptionKeyS.begin(), encryptionKeyS.end(), std::back_inserter(encryptionKey));
            }

            UniValue socks5ProxyU = find_value(objData, kSOCKS5_PROXY);
            if (socks5ProxyU.isStr())
                socks5ProxyURL = socks5ProxyU.get_str();

            UniValue dbbProxyU = find_value(objData, kDBB_PROXY);
            if (dbbProxyU.isBool())
                dbbProxy = dbbProxyU.get_bool();
            else
                dbbProxy = false;

            UniValue defaultProxyU = find_value(objData, kUSE_DEFAULT_PROXY);
            if (defaultProxyU.isBool())
                useDefaultProxy = defaultProxyU.get_bool();
            else
                useDefaultProxy = false;
        }


        return true;
    }
};
}
#endif //DBB_CONFIGDATA_H
