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


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>

#include <string>

#include <hidapi/hidapi.h>
#include <openssl/aes.h>
#include <openssl/bio.h>
#include <openssl/buffer.h>
#include <openssl/evp.h>
#include "openssl/sha.h"

#pragma clang diagnostic ignored "-Wdeprecated-declarations"

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

class CDBBCommand
{
public:
    std::string cmdname;
    std::string json;
    bool needsEncryption;
};

static const CDBBCommand vCommands[] =
{
    { "erase"           , "{\"reset\" : \"__ERASE__\"}",            false},
    { "password"           , "{\"password\" : \"0000\"}",            false},
    { "led"             , "{\"led\" : \"toggle\"}",                 true}
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
    res = hid_read(HID_HANDLE, HID_REPORT, HID_REPORT_SIZE);
	printf(" OK, read %d bytes.\n", res);
	printf("Result: %s\n", HID_REPORT);

    return true;
}

void doubleSha256(char *string, unsigned char *hashOut)
{
    unsigned char firstSha[SHA256_DIGEST_LENGTH];
    SHA256_CTX sha256;
    SHA256_Init(&sha256);
    SHA256_Update(&sha256, string, strlen(string));
    SHA256_Final(firstSha, &sha256);

    SHA256_Init(&sha256);
    SHA256_Update(&sha256, firstSha, SHA256_DIGEST_LENGTH);
    SHA256_Final(hashOut, &sha256);
}

class Crypto
{
private:
    EVP_CIPHER_CTX *aesEncryptCtx;
    EVP_CIPHER_CTX *aesDecryptCtx;
public:
    unsigned char aesKey[16];
    unsigned char aesIV[16];

    int aesEncrypt(const unsigned char *msg, size_t msgLen, unsigned char **encMsg);
    std::string base64encode(const unsigned char *msg, int size);
};

int Crypto::aesEncrypt(const unsigned char *msg, size_t msgLen, unsigned char **encMsg) {
    size_t blockLen  = 0;
    size_t encMsgLen = 0;

    aesEncryptCtx = (EVP_CIPHER_CTX*)malloc(sizeof(EVP_CIPHER_CTX));
    EVP_CIPHER_CTX_init(aesEncryptCtx);

    *encMsg = (unsigned char*)malloc(msgLen + AES_BLOCK_SIZE);
    if(encMsg == NULL) return FAILURE;

    if(!EVP_EncryptInit_ex(aesEncryptCtx, EVP_aes_256_cbc(), NULL, aesKey, aesIV)) {
        return FAILURE;
    }

    if(!EVP_EncryptUpdate(aesEncryptCtx, *encMsg, (int*)&blockLen, (unsigned char*)msg, msgLen)) {
        return FAILURE;
    }
    encMsgLen += blockLen;

    if(!EVP_EncryptFinal_ex(aesEncryptCtx, *encMsg + encMsgLen, (int*)&blockLen)) {
        return FAILURE;
    }

    EVP_CIPHER_CTX_cleanup(aesEncryptCtx);

    return encMsgLen + blockLen;
}

std::string Crypto::base64encode(const unsigned char *msg, int size)
{

    BIO *bio, *b64;
    BUF_MEM *bufferPtr;

    b64 = BIO_new(BIO_f_base64());
    bio = BIO_new(BIO_s_mem());
    bio = BIO_push(b64, bio);

    BIO_set_flags(bio, BIO_FLAGS_BASE64_NO_NL); //Ignore newlines - write everything in one line
    BIO_write(bio, msg, size);
    BIO_flush(bio);
    BIO_get_mem_ptr(bio, &bufferPtr);
    BIO_set_close(bio, BIO_NOCLOSE);
    BIO_free_all(bio);

    std::string s((*bufferPtr).data);
    return s;
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

                if (cmd.needsEncryption)
                {
                    unsigned char outputBuffer[SHA256_DIGEST_LENGTH];
                    doubleSha256((char *)pasword.c_str(), (unsigned char *)&outputBuffer);

                    unsigned char *cypher;
                    Crypto nC;
                    memcpy(nC.aesIV, &outputBuffer, SHA256_DIGEST_LENGTH);
                    memcpy(nC.aesKey, &outputBuffer, SHA256_DIGEST_LENGTH);
                    int newLen = nC.aesEncrypt((const unsigned char *)cmd.json.c_str(), cmd.json.size(),&cypher);

                    unsigned char *newB = (unsigned char *)malloc(newLen+16);
                    memcpy(newB, nC.aesIV, 16);
                    memcpy(newB+16, cypher, newLen);

                    std::string base64str = nC.base64encode(newB, newLen+16);
                    sendCommand(base64str, cmdOut);
                }
                else
                {
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
