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
#include <fstream>
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>

#include <string>

#include "dbb.h"
#include "dbb_util.h"

#include "univalue.h"
#include "hidapi/hidapi.h"

#include <btc/hash.h>
#include <btc/ecc_key.h>
#include <btc/ecc.h>

//simple class for a dbb command
class CDBBCommand
{
public:
    std::string cmdname;
    std::string json;
    std::string example;
    bool requiresEncryption;
};

//dispatch table
// %var% will be replace with a corresponding command line agrument with a leading -
//  example -password=0000 will result in a input json {"%password%"} being replace to {"0000"}
//
// variables with no corresponding command line argument will be replaced with a empty string
// variables with a leading ! are mandatory and therefore missing a such will result in an error
static const CDBBCommand vCommands[] =
{
    { "erase"             , "{\"reset\" : \"__ERASE__\"}",                                "", false},
    { "password"          , "{\"password\" : \"%!newpassword%\"}",                        "", false},
    { "led"               , "{\"led\" : \"blink\"}",                                      "", true},
    { "seed"              , "{\"seed\" : {"
                                "\"source\" :\"%source|create%\","
                                "\"raw\": \"%raw|false%\","
                                "\"entropy\": \"%entropy|0123456789abcde%\","
                                "\"key\": \"%key|1234%\","
                                "\"filename\": \"%filename|backup.dat%\"}"
                            "}",                                                          "", true},
    { "backuplist"        , "{\"backup\" : \"list\"}",                                    "", true},
    { "backuperaseall"    , "{\"backup\" : \"erase\"}",                                   "", true},
    { "backuperasefile"   , "{\"backup\" : {\"erase\":\"%!filename%\"}}",                 "", true},
    { "backup"            , "{\"backup\" : {"
                                "\"key\":\"%key|1234%\","
                                "\"filename\": \"%filename|backup.dat%\"}"
                            "}",                                                          "", true},
    { "sign"              , "{\"sign\" : {"
                                "\"pin\" : \"%lock_pin|1234%\","
                                "\"meta\" : \"%meta|meta%\","
                                "\"data\" : %!hasharray%,"
                                "\"checkpub\": %pubkeyarray|[{\"pubkey\":\"000000000000000000000000000000000000000000000000000000000000000000\", \"keypath\":\"m/44\"}]% }"
                            "}",
        "dbb-cli --password=0000 sign -hasharray='[{\"hash\": \"f6f4a3633eda92eef9dd96858dec2f5ea4dfebb67adac879c964194eb3b97d79\", \"keypath\":\"m/44/0\"}]' -pubkeyarray='[{\"pubkey\":\"0270526bf580ddb20ad18aad62b306d4beb3b09fae9a70b2b9a93349b653ef7fe9\", \"keypath\":\"m/44\"}]' -meta=34982a264657cdf2635051bd778c99e73ce5eb2e8c7f9d32b8aaa7e547c7fd90\n\n(This signs the given hash(es) with the privatekey specified at keypath. It checks if the pubkey at a given keypath is equal/valid.",                                       true},

    { "xpub"              , "{\"xpub\" : \"%!keypath%\"}",                                "", true},
    { "random"            , "{\"random\" : \"%mode|true%\"}",                             "", true},
    { "info"              , "{\"device\" : \"info\"}",                                    "", true},

    { "lock"              , "{\"device\" : \"lock\"}",                                    "", true},
    { "verifypass"        , "{\"verifypass\" : \"%operation|create%\"}",                  "", true},

    { "aes"               , "{\"aes256cbc\" : {"
                                "\"type\":\"%type|encrypt%\","
                                "\"data\": \"%!data%\"}"
                            "}",                                                          "", true},
    { "bootloaderunlock"  , "{\"bootloader\" : \"unlock\"}",                              "", true},
    { "bootloaderlock"    , "{\"bootloader\" : \"lock\"}",                                "", true},
    { "firmware"          , "%filename%",                                                 "", true},
    /*{ "decryptbackup"     , "%filename%",                                                 "", true},*//* no longer valid for firmware v2 */
    { "hidden_password"   , "{\"hidden_password\" : \"%!hiddenpassword%\"}",              "", true},
    { "u2f-on"            , "{\"feature_set\" : {\"U2F\": true} }",                       "", true},
    { "u2f-off"           , "{\"feature_set\" : {\"U2F\": false} }",                      "", true},
};


#define strlens(s) (s == NULL ? 0 : strlen(s))
#define PBKDF2_SALT     "Digital Bitbox"
#define PBKDF2_SALTLEN  14
#define PBKDF2_ROUNDS   2048
#define PBKDF2_HMACLEN  64

extern "C" {
    extern void hmac_sha512(const uint8_t* key, const uint32_t keylen, const uint8_t* msg, const uint32_t msglen, uint8_t* hmac);
    extern char* utils_uint8_to_hex(const uint8_t* bin, size_t l);
}
void pbkdf2_hmac_sha512(const uint8_t *pass, int passlen, uint8_t *key, int keylen)
{
    uint32_t i, j, k;
    uint8_t f[PBKDF2_HMACLEN], g[PBKDF2_HMACLEN];
    uint32_t blocks = keylen / PBKDF2_HMACLEN;

    static uint8_t salt[PBKDF2_SALTLEN + 4];
    memset(salt, 0, sizeof(salt));
    memcpy(salt, PBKDF2_SALT, strlens(PBKDF2_SALT));

    if (keylen & (PBKDF2_HMACLEN - 1)) {
        blocks++;
    }
    for (i = 1; i <= blocks; i++) {
        salt[PBKDF2_SALTLEN    ] = (i >> 24) & 0xFF;
        salt[PBKDF2_SALTLEN + 1] = (i >> 16) & 0xFF;
        salt[PBKDF2_SALTLEN + 2] = (i >> 8) & 0xFF;
        salt[PBKDF2_SALTLEN + 3] = i & 0xFF;
        hmac_sha512(pass, passlen, salt, PBKDF2_SALTLEN + 4, g);
        memcpy(f, g, PBKDF2_HMACLEN);
        for (j = 1; j < PBKDF2_ROUNDS; j++) {
            hmac_sha512(pass, passlen, g, PBKDF2_HMACLEN, g);
            for (k = 0; k < PBKDF2_HMACLEN; k++) {
                f[k] ^= g[k];
            }
        }
        if (i == blocks && (keylen & (PBKDF2_HMACLEN - 1))) {
            memcpy(key + PBKDF2_HMACLEN * (i - 1), f, keylen & (PBKDF2_HMACLEN - 1));
        } else {
            memcpy(key + PBKDF2_HMACLEN * (i - 1), f, PBKDF2_HMACLEN);
        }
    }
    memset(f, 0, sizeof(f));
    memset(g, 0, sizeof(g));
}

int main(int argc, char* argv[])
{
    DBB::ParseParameters(argc, argv);

    unsigned int i = 0, loop = 0;
    bool cmdfound = false;
    std::string userCmd;

    //search after first argument with no -
    std::vector<std::string> cmdArgs;
    for (int i = 1; i < argc; i++)
        if (strlen(argv[i]) > 0 && argv[i][0] != '-')
            cmdArgs.push_back(std::string(argv[i]));

    if (cmdArgs.size() > 0)
        userCmd = cmdArgs.front();

    if (userCmd == "help" || DBB::mapArgs.count("-help") || userCmd == "?") {
        printf("Usage: %s -<arg0>=<value> -<arg1>=<value> ... <command>\n\nAvailable commands with possible arguments (* = mandatory):\n", "dbb_cli");
        //try to find the command in the dispatch table
        for (i = 0; i < (sizeof(vCommands) / sizeof(vCommands[0])); i++) {
            CDBBCommand cmd = vCommands[i];
            printf("  %s ", cmd.cmdname.c_str());

            std::string json = cmd.json;
            int tokenOpenPos = -1;
            int defaultDelimiterPos = -1;
            std::string var;
            std::string defaultValue;

            bool first = true;
            for (unsigned int sI = 0; sI < json.length(); sI++) {
                if (json[sI] == '|' && tokenOpenPos > 0)
                    defaultDelimiterPos = sI;

                if (json[sI] == '%' && tokenOpenPos < 0)
                    tokenOpenPos = sI;

                else if (json[sI] == '%' && tokenOpenPos >= 0) {
                    defaultValue = "";
                    if (defaultDelimiterPos >= 0) {
                        var = json.substr(tokenOpenPos + 1, defaultDelimiterPos - tokenOpenPos - 1);
                        defaultValue = json.substr(defaultDelimiterPos + 1, sI - defaultDelimiterPos - 1);
                    } else {
                        var = json.substr(tokenOpenPos + 1, sI - tokenOpenPos - 1);
                    }

                    tokenOpenPos = -1;
                    defaultDelimiterPos = -1;

                    if (!first)
                        printf(", ");
                    first = false;

                    bool mandatory = false;
                    if (var.size() > 0 && var[0] == '!') {
                        var.erase(0, 1);
                        mandatory = true;
                    }

                    if (!defaultValue.empty())
                        printf("-%s (default: %s)", var.c_str(), defaultValue.c_str());
                    else if (mandatory)
                        printf("-*%s", var.c_str());
                    else
                        printf("-%s", var.c_str());
                }
            }
            printf("\n");
            if (cmd.example.size() > 0)
                printf("\n    Example\n    =======\n    %s\n\n", cmd.example.c_str());

            continue;
        }
        return 1;
    }
    std::string devicePath;
    enum DBB::dbb_device_mode deviceMode = DBB::deviceAvailable(devicePath);
    if (userCmd == "firmware" && deviceMode != DBB::DBB_DEVICE_MODE_BOOTLOADER)
    {
        printf("Error: No Digital Bitbox is Bootloader-Mode detected\n");
        return 1;
    }
    bool connectRes = DBB::openConnection(deviceMode, devicePath);

    if (!connectRes)
        printf("Error: No Digital Bitbox connected\n");
    else {
        DebugOut("main", "Digital Bitbox connected\n");

        if (argc < 2) {
            printf("no command given\n");
            return 0;
        }

        if (userCmd == "decryptbackup")
        {
            if (!DBB::mapArgs.count("-password"))
            {
                printf("You need to provide the password used during backup creation (-password=<password>)\n");
                return 0;
            }

            std::string password = DBB::GetArg("-password", "0000");
            std::string key;

            // load the file
            std::string possibleFilename = DBB::mapArgs["-filename"];
            if ((possibleFilename.empty() || possibleFilename == ""))
                possibleFilename = cmdArgs[1].c_str();

            std::ifstream backupFile(possibleFilename, std::ios::ate);
            std::streamsize backupSize = backupFile.tellg();

            if (backupSize <= 0)
            {
                printf("Backup file (%s) does not exists or is empty\n", possibleFilename.c_str());
                return 0;
            }
            backupFile.seekg(0, std::ios::beg);

            //PBKDF2 key stretching
            key.resize(PBKDF2_HMACLEN);
            pbkdf2_hmac_sha512((uint8_t *)password.c_str(), password.size(), (uint8_t *)&key[0], PBKDF2_HMACLEN);

            std::string backupBuffer((std::istreambuf_iterator<char>(backupFile)), std::istreambuf_iterator<char>());
            backupBuffer = "{\"ciphertext\" : \""+backupBuffer+"\"}";
            std::string unencryptedBackup;
            DBB::decryptAndDecodeCommand(backupBuffer, std::string(utils_uint8_to_hex((uint8_t *)&key[0], PBKDF2_HMACLEN)), unencryptedBackup);
            printf("%s\n", unencryptedBackup.c_str());
        }
        else if (userCmd == "firmware")
        {
            // dummy private key to allow current testing
            // the private key matches the pubkey on the DBB bootloader / FW
            std::string testing_privkey = "e0178ae94827844042d91584911a6856799a52d89e9d467b83f1cf76a0482a11";

            // load the file
            std::string possibleFilename = DBB::mapArgs["-filename"];
            if (possibleFilename.empty() || possibleFilename == "")
                possibleFilename = cmdArgs[1].c_str();

            std::ifstream firmwareFile(possibleFilename, std::ios::binary | std::ios::ate);
            std::streamsize firmwareSize = firmwareFile.tellg();
            if (firmwareSize > 0)
            {
                firmwareFile.seekg(0, std::ios::beg);

                std::vector<char> firmwareBuffer(DBB_APP_LENGTH);
                unsigned int pos = 0;
                while (true)
                {
                    //read into
                    firmwareFile.read(&firmwareBuffer[0]+pos, FIRMWARE_CHUNKSIZE);
                    std::streamsize bytes = firmwareFile.gcount();
                    if (bytes == 0)
                        break;

                    pos += bytes;
                }
                firmwareFile.close();

                // append 0xff to the rest of the firmware buffer
                memset((void *)(&firmwareBuffer[0]+pos), 0xff, DBB_APP_LENGTH-pos);

                // generate a double SHA256 of the firmware data
                uint8_t hashout[32];
                btc_hash((const uint8_t*)&firmwareBuffer[0], firmwareBuffer.size(), hashout);
                std::string hashHex = DBB::HexStr(hashout, hashout+32);

                // sign and get the compact signature
                btc_ecc_start();
                btc_key key;
                btc_privkey_init(&key);
                std::vector<unsigned char> privkey = DBB::ParseHex(testing_privkey);
                memcpy(&key.privkey, &privkey[0], 32);

                size_t sizeout = 64;
                unsigned char sig[sizeout];
                int res = btc_key_sign_hash_compact(&key, hashout, sig, &sizeout);
                std::string sigStr = DBB::HexStr(sig, sig+sizeout);

                btc_ecc_stop();
                // send firmware blob to DBB
                if (!DBB::upgradeFirmware(firmwareBuffer, firmwareSize, sigStr, [](const std::string& infotext, float progress) {}))
                    printf("Firmware upgrade failed!\n");
                else
                    printf("Firmware successfully upgraded!\n");
            }
            else
                printf("Can't open firmware file!\n");
        }
        else {
            //try to find the command in the dispatch table
            for (i = 0; i < (sizeof(vCommands) / sizeof(vCommands[0])); i++) {
                CDBBCommand cmd = vCommands[i];
                if (cmd.cmdname == userCmd) {
                    std::string cmdOut;
                    std::string json = cmd.json;
                    size_t index = 0;

                    //replace %vars% in json string with cmd args

                    int tokenOpenPos = -1;
                    int defaultDelimiterPos = -1;
                    std::string var;
                    std::string defaultValue;

                    for (unsigned int sI = 0; sI < json.length(); sI++) {
                        if (json[sI] == '|' && tokenOpenPos > 0)
                            defaultDelimiterPos = sI;

                        if (json[sI] == '%' && tokenOpenPos < 0)
                            tokenOpenPos = sI;

                        else if (json[sI] == '%' && tokenOpenPos >= 0) {
                            if (defaultDelimiterPos >= 0) {
                                var = json.substr(tokenOpenPos + 1, defaultDelimiterPos - tokenOpenPos - 1);
                                defaultValue = json.substr(defaultDelimiterPos + 1, sI - defaultDelimiterPos - 1);
                            } else {
                                var = json.substr(tokenOpenPos + 1, sI - tokenOpenPos - 1);
                            }

                            //always erase
                            json.erase(tokenOpenPos, sI - tokenOpenPos + 1);

                            bool mandatory = false;
                            if (var.size() > 0 && var[0] == '!') {
                                var.erase(0, 1);
                                mandatory = true;
                            }

                            var = "-" + var; //cmd args come in over "-arg"
                            if (DBB::mapArgs.count(var)) {
                                json.insert(tokenOpenPos, DBB::mapArgs[var]);
                            } else if (mandatory) {
                                printf("Argument %s is mandatory for command %s\n", var.c_str(), cmd.cmdname.c_str());
                                return 0;
                            } else if (defaultDelimiterPos > 0 && !defaultValue.empty())
                                json.insert(tokenOpenPos, defaultValue);

                            tokenOpenPos = -1;
                            defaultDelimiterPos = -1;
                        }
                    }

                    if (cmd.requiresEncryption || DBB::mapArgs.count("-password")) {
                        if (!DBB::mapArgs.count("-password")) {
                            printf("This command requires the -password argument\n");
                            DBB::closeConnection();
                            return 0;
                        }

                        if (!cmd.requiresEncryption) {
                            DebugOut("main", "Using encyption because -password was set\n");
                        }

                        std::string password = DBB::GetArg("-password", "0000"); //0000 will never be used because setting a password is required
                        std::string base64str;
                        std::string unencryptedJson;

                        DebugOut("main", "encrypting raw json: %s\n", json.c_str());
                        DBB::encryptAndEncodeCommand(json, password, base64str);
                        DBB::sendCommand(base64str, cmdOut);
                        try {
                            //hack: decryption needs the new password in case the AES256CBC password has changed
                            if (DBB::mapArgs.count("-newpassword"))
                                password = DBB::GetArg("-newpassword", "");

                            UniValue testJson;
                            testJson.read(cmdOut);
                            UniValue ct;

                            if (testJson.isObject())
                                ct = find_value(testJson, "ciphertext");

                            if (testJson.isObject() && !ct.isStr())
                            {
                                //json was unencrypted
                                unencryptedJson = cmdOut;
                            }
                            else
                                DBB::decryptAndDecodeCommand(cmdOut, password, unencryptedJson);
                        } catch (const std::exception& ex) {
                            printf("%s\n", ex.what());
                            DBB::closeConnection();
                            exit(0);
                        }

                        //example json en/decode
                        UniValue json;
                        json.read(unencryptedJson);
                        std::string jsonFlat = json.write(2); //pretty print with a intend of 2
                        printf("result: %s\n", jsonFlat.c_str());
                    } else {
                        //send command unencrypted
                        DBB::sendCommand(json, cmdOut);
                        printf("result: %s\n", cmdOut.c_str());
                    }
                    cmdfound = true;
                }
            }

            if (!cmdfound) {
                //try to send it as raw json
                if (userCmd.size() > 1 && userCmd.at(0) == '{') //todo: ignore whitespace
                {
                    std::string cmdOut;
                    DebugOut("main", "Send raw json %s\n", userCmd.c_str());
                    cmdfound = DBB::sendCommand(userCmd, cmdOut);
                }

                printf("command (%s) not found, use \"help\" to list available commands\n", DBB::SanitizeString(userCmd).c_str());
            }
        }

        DBB::closeConnection();
    }

    return 0;
}
