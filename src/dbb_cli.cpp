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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>

#include <string>

#include <hidapi/hidapi.h>
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

/* 
    currently avoid headers 
    define everything as extern
*/
extern std::string base64_encode(unsigned char const* bytes_to_encode, unsigned int in_len);
extern std::string base64_decode(std::string const& encoded_string);
extern int aesDecrypt(unsigned char *aesKey, unsigned char *aesIV, unsigned char *encMsg, size_t encMsgLen, unsigned char **decMsg);
extern int aesEncrypt(unsigned char *aesKey, unsigned char *aesIV, const unsigned char *msg, size_t msgLen, unsigned char **encMsg);
extern void doubleSha256(char *string, unsigned char *hashOut);
extern void getRandIV(unsigned char *ivOut);

#define AES_BLOCKSIZE 16

static const CDBBCommand vCommands[] =
{
    { "erase"           , "{\"reset\" : \"__ERASE__\"}",                            false},
    { "password"        , "{\"password\" : \"0000\"}",                              false},
    { "led"             , "{\"led\" : \"toggle\"}",                                 true},
    { "seed"            , "{\"seed\" : {\"source\" : \"create\"} }",                true},
};

bool sendCommand(const std::string &json, std::string &resultOut)
{
    int res;

    printf("Sending command: %s\n", json.c_str());

    memset(HID_REPORT, 0, HID_REPORT_SIZE);
    memcpy(HID_REPORT, json.c_str(), json.size() );
    hid_write(HID_HANDLE, (unsigned char *)HID_REPORT, HID_REPORT_SIZE);

    memset(HID_REPORT, 0, HID_REPORT_SIZE);
    printf("try to read some bytes...\n");
    res = hid_read(HID_HANDLE, (unsigned char *)&resultOut[0], HID_REPORT_SIZE);
	printf(" OK, read %d bytes.\n", res);
    
	printf("Result: %s\n", resultOut.c_str());
    return true;
}

int main( int argc, char *argv[] )
{
    if (api_hid_init() == ERROR) {
        printf("No digital bitbox connected");
		return 0;
    } else {
        printf("Digital Bitbox Connected\n");

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
                std::string pasword = "0000";
                std::string cmdOut;

                if (cmd.requiresEncryption)
                {
                    //double sha256 the password
                    unsigned char outputBuffer[SHA256_DIGEST_LENGTH];
                    doubleSha256((char *)pasword.c_str(), outputBuffer);
                    
                    unsigned char *cypher;
                    unsigned char aesIV[AES_BLOCKSIZE];
                    unsigned char aesKey[AES_BLOCKSIZE];
                    
                    getRandIV(aesIV);
                    memcpy(aesKey, outputBuffer, SHA256_DIGEST_LENGTH);
                    
                    int inlen = cmd.json.size();
                    int  pads;
                    int  inpadlen = inlen + AES_BLOCKSIZE - inlen % AES_BLOCKSIZE;
                    unsigned char inpad[inpadlen];
                    unsigned char enc[inpadlen];
                    unsigned char enc_cat[inpadlen + AES_BLOCKSIZE]; // concatenating [ iv0  |  enc ]
                    
                    // PKCS7 padding
                    memcpy(inpad, cmd.json.c_str(), inlen);
                    for (pads = 0; pads < AES_BLOCKSIZE - inlen % AES_BLOCKSIZE; pads++ ) {
                        inpad[inlen + pads] = (AES_BLOCKSIZE - inlen % AES_BLOCKSIZE);
                    }
                    
                    //add iv to the stream for base64 encoding
                    memcpy(enc_cat, aesIV, AES_BLOCKSIZE);
                    
                    //encrypt
                    aesEncrypt(aesKey, aesIV, inpad, cmd.json.size(), &cypher);
                    
                    //copy the encypted data to the stream where the iv is already
                    memcpy(enc_cat + AES_BLOCKSIZE, cypher, inpadlen);

                    //base64 encode
                    std::string base64str = base64_encode(enc_cat, inpadlen + AES_BLOCKSIZE);
                    
                    
                    // DECRIPT FOR SANITY REASONS
                    //test, decode
                    printf("base64 cmd: %s\n", base64str.c_str());
                    std::string base64Dec = base64_decode(base64str);
                    
                    //test decrypt
                    unsigned char *decryptedStream;
                    unsigned char *decryptedCommand;
                    int outlen = aesDecrypt(aesKey, aesIV, (unsigned char *)base64Dec.c_str(), base64Dec.size(), &decryptedStream);
                    decryptedCommand = (unsigned char *)malloc(outlen);
                    memcpy(decryptedCommand, decryptedStream+16, outlen-16);
                    decryptedCommand[outlen-16] =0;
                    
                    printf("\n\n1: %s, 2: %s\n\n", decryptedCommand, cmd.json.c_str());
                    assert(strcmp((const char *)decryptedCommand, cmd.json.c_str()) == 0);
                    sendCommand(base64str, cmdOut);
                    
                    //decrypt result: TODO:
                }
                else
                {
                    //send command unencrypted
        	        sendCommand(cmd.json,cmdOut);
                }
                cmdfound = true;
            }
        }

        if (!cmdfound)
        {
            //try to send it as raw json
            if (userCmd.size() > 1 && userCmd.front() == '{') //todo: ignore whitespace
            {
                printf("Send raw json %s\n", userCmd.c_str());
                std::string cmdOut;
        	    cmdfound = sendCommand(userCmd,cmdOut);
            }

            printf("command not found\n");
        }
    }
    return 0;
}
