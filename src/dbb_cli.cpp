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

#include "util.h"
#include "crypto.h"

#include "univalue/univalue.h"
#include "hidapi/hidapi.h"
#include "openssl/sha.h"

#define HID_REPORT_SIZE   2048
#define ERROR 0
#define SUCCESS 1
#define FAILURE 0

static hid_device *HID_HANDLE;
static unsigned char HID_REPORT[HID_REPORT_SIZE] = {0};

static int api_hid_init(void)
{
    HID_HANDLE = hid_open(0x03eb, 0x2402, NULL);
    if (!HID_HANDLE) {
        return ERROR;
    }
    return SUCCESS;
}

//simple dispatch class for a dbb command
class CDBBCommand
{
public:
    std::string cmdname;
    std::string json;
    bool requiresEncryption;
};

static const CDBBCommand vCommands[] =
{
    { "erase"           , "{\"reset\" : \"__ERASE__\"}",                            false},
    { "password"        , "{\"password\" : \"0000\"}",                              false},
    { "led"             , "{\"led\" : \"toggle\"}",                                 true},
    { "seed"            , "{\"seed\" : {\"source\" : \"create\"} }",                true},
};

bool sendCommand(const std::string &json, std::string &resultOut)
{
    int res, cnt = 0;

    printf("Sending command: %s\n", json.c_str());

    memset(HID_REPORT, 0, HID_REPORT_SIZE);
    memcpy(HID_REPORT, json.c_str(), json.size() );
    hid_write(HID_HANDLE, (unsigned char *)HID_REPORT, HID_REPORT_SIZE);

    memset(HID_REPORT, 0, HID_REPORT_SIZE);
    printf("try to read some bytes...\n");
    
    memset(HID_REPORT, 0, HID_REPORT_SIZE);
    while (cnt < HID_REPORT_SIZE) {
        res = hid_read(HID_HANDLE, HID_REPORT + cnt, HID_REPORT_SIZE);
        if (res < 0) {
            printf("ERROR: Unable to read report.\n");
            return false;
        }
        cnt += res;
    }
        
	printf(" OK, read %d bytes.\n", res);

    resultOut.assign((const char *)HID_REPORT);
    return true;
}

bool decryptAndDecodeCommand(const std::string &cmdIn, const std::string &password, UniValue &jsonOut)
{
    unsigned char passwordSha256[SHA256_DIGEST_LENGTH];
    unsigned char aesIV[DBB_AES_BLOCKSIZE];
    unsigned char aesKey[DBB_AES_KEYSIZE];

    doubleSha256((char *)password.c_str(), passwordSha256);
    memcpy(aesKey, passwordSha256, DBB_AES_KEYSIZE);
    
    //decrypt result: TODO:
    UniValue valRead(UniValue::VSTR);
    if (!valRead.read(cmdIn))
    {
        printf("could not deserialize json\n");
        return false;
    }
    UniValue ctext = find_value(valRead, "ciphertext");
    if (ctext.isNull())
    {
        printf("No ciphertext in response found. Aborting.\n");
        return false;
    }

    std::string base64dec = base64_decode(ctext.get_str());
    unsigned int base64_len = base64dec.size();
    unsigned char *base64dec_c = (unsigned char *)base64dec.c_str();

    unsigned char *decryptedStream;
    unsigned char *decryptedCommand;
    memcpy(aesIV, base64dec_c, DBB_AES_BLOCKSIZE); //copy first 16 bytes and take as IV
    int outlen = aesDecrypt(aesKey, aesIV, base64dec_c+DBB_AES_BLOCKSIZE, base64_len-DBB_AES_BLOCKSIZE, &decryptedStream);

    int decrypt_len = 0;
    int padlen = decryptedStream[base64_len - DBB_AES_BLOCKSIZE - 1];
    char *dec = (char *)malloc(base64_len - DBB_AES_BLOCKSIZE - padlen + 1); // +1 for null termination
    if (!dec) {
        decrypt_len = 0;
        memset(decryptedStream, 0, sizeof(&decryptedStream));
        return false;
    }
    memcpy(dec, decryptedStream, base64_len - DBB_AES_BLOCKSIZE - padlen);
    dec[base64_len - DBB_AES_BLOCKSIZE - padlen] = 0;
    decrypt_len = base64_len - DBB_AES_BLOCKSIZE - padlen + 1;
    memset(decryptedStream, 0, sizeof(&decryptedStream));

    return true;
}

bool encryptAndEncodeCommand(const std::string &cmd, const std::string &password, std::string &base64strOut)
{
    if (password.empty())
        return false;

    //double sha256 the password
    unsigned char passwordSha256[SHA256_DIGEST_LENGTH];
    unsigned char *cypher;
    unsigned char aesIV[DBB_AES_BLOCKSIZE];
    unsigned char aesKey[DBB_AES_KEYSIZE];

    doubleSha256((char *)password.c_str(), passwordSha256);

    //set random IV
    getRandIV(aesIV);
    memcpy(aesKey, passwordSha256, DBB_AES_KEYSIZE);

    int inlen = cmd.size();
    unsigned int pads = 0;
    int inpadlen = inlen + DBB_AES_BLOCKSIZE - inlen % DBB_AES_BLOCKSIZE;
    unsigned char inpad[inpadlen];
    unsigned char enc[inpadlen];
    unsigned char enc_cat[inpadlen + DBB_AES_BLOCKSIZE]; // concatenating [ iv0  |  enc ]

    // PKCS7 padding
    memcpy(inpad, cmd.c_str(), inlen);
    for (pads = 0; pads < DBB_AES_BLOCKSIZE - inlen % DBB_AES_BLOCKSIZE; pads++ ) {
        inpad[inlen + pads] = (DBB_AES_BLOCKSIZE - inlen % DBB_AES_BLOCKSIZE);
    }

    //add iv to the stream for base64 encoding
    memcpy(enc_cat, aesIV, DBB_AES_BLOCKSIZE);

    //encrypt
    aesEncrypt(aesKey, aesIV, inpad, inlen, &cypher);

    //copy the encypted data to the stream where the iv is already
    memcpy(enc_cat + DBB_AES_BLOCKSIZE, cypher, inpadlen);

    //base64 encode
    base64strOut = base64_encode(enc_cat, inpadlen + DBB_AES_BLOCKSIZE);

    return true;
}

int main( int argc, char *argv[] )
{
    if (api_hid_init() == ERROR) {
        printf("No digital bitbox connected");
		return 0;
    } else {
        LogPrint("main", "Digital Bitbox Connected\n");

        if (argc < 2)
        {
            printf("no command given\n");
            return 0;
        }
        unsigned int i = 0;
        bool cmdfound = false;
        std::string userCmd = std::string(argv[1]);

        for (i = 0; i < (sizeof(vCommands) / sizeof(vCommands[0])); i++)
        {
            CDBBCommand cmd = vCommands[i];
            if (cmd.cmdname == userCmd)
            {
                std::string password = "0000";
                std::string cmdOut;

                if (cmd.requiresEncryption)
                {
                    std::string base64str;
                    encryptAndEncodeCommand(cmd.json, password, base64str);
                    
                    sendCommand(base64str, cmdOut);
                    
                    UniValue json;
                    decryptAndDecodeCommand(cmdOut, password, json);
                    std::string jsonFlat = json.write(2);
                    LogPrint("main", jsonFlat.c_str());
                }
                else
                {
                    //send command unencrypted
        	        sendCommand(cmd.json, cmdOut);
                    printf("  result: %s\n", cmdOut.c_str());
                }
                cmdfound = true;
            }
        }

        if (!cmdfound)
        {
            //try to send it as raw json
            if (userCmd.size() > 1 && userCmd.at(0) == '{') //todo: ignore whitespace
            {
                std::string cmdOut;
                printf("Send raw json %s\n", userCmd.c_str());
        	    cmdfound = sendCommand(userCmd, cmdOut);
            }

            LogPrintStr("command ("+SanitizeString(userCmd)+") not found\n");
        }
    }
    return 0;
}
