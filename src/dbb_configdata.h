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
static const char *comServerDefaultURL = "https://bitcoin.jonasschnelli.ch/dbb/server.php";

static const char *kVERSION = "version";
static const char *kCOM_CHANNEL_ID = "comserverchannelid";
static const char *kENC_PKEY = "encryptionprivkey";
static const char *kBWS_URL = "bws_url";
static const char *kCOMSERVER_URL = "comsrv_url";

namespace DBB
{

// simple model/controller class for persistance use settings
// uses JSON/file as persistance store
class DBBConfigdata {
public:
    static const int CURRENT_VERSION=1;
    std::string filename;

    int32_t version;
    std::string comServerURL;
    std::string bwsBackendURL;
    std::string comServerChannelID;
    std::vector<unsigned char> encryptionKey;

    DBBConfigdata(const std::string& filenameIn)
    {
        bwsBackendURL = std::string(bwsDefaultBackendURL);
        comServerURL = std::string(comServerDefaultURL);
        filename = filenameIn;
        version = CURRENT_VERSION;
        encryptionKey.clear();
        encryptionKey.resize(32);
        memset(&encryptionKey[0], 0, 32);
    }

    bool write()
    {
        UniValue objData(UniValue::VOBJ);
        objData.pushKV(kVERSION, version);
        objData.pushKV(kENC_PKEY, base64_encode(&encryptionKey[0], 32));
        objData.pushKV(kCOM_CHANNEL_ID, comServerChannelID);
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
                printf("%s", encryptionKeyS.c_str());
                encryptionKey.clear();
                std::copy(encryptionKeyS.begin(), encryptionKeyS.end(), std::back_inserter(encryptionKey));
            }
        }


        return true;
    }
};
}
#endif //DBB_CONFIGDATA_H
