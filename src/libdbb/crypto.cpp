// Copyright (c) 2015 Jonas Schnelli
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "crypto.h"

#include <openssl/aes.h>
#include <openssl/bio.h>
#include <openssl/buffer.h>
#include <openssl/evp.h>
#include <openssl/sha.h>
#include <openssl/rand.h>

#include <string.h>

//ignore osx depracation warning
#pragma clang diagnostic ignored "-Wdeprecated-declarations"

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

int aesDecrypt(unsigned char *aesKey, unsigned char *aesIV, unsigned char *encMsg, size_t encMsgLen, unsigned char **decMsg) {
    size_t decLen   = 0;
    size_t blockLen = 0;
 
    EVP_CIPHER_CTX *aesDecryptCtx = (EVP_CIPHER_CTX*)malloc(sizeof(EVP_CIPHER_CTX));
    EVP_CIPHER_CTX_init(aesDecryptCtx);
    
    *decMsg = (unsigned char*)malloc(encMsgLen);
    if(*decMsg == NULL) return false;
 
    if(!EVP_DecryptInit_ex(aesDecryptCtx, EVP_aes_256_cbc(), NULL, aesKey, aesIV)) {
        return false;
    }
 
    if(!EVP_DecryptUpdate(aesDecryptCtx, (unsigned char*)*decMsg, (int*)&blockLen, encMsg, (int)encMsgLen)) {
        return false;
    }
    decLen += blockLen;
 
    if(!EVP_DecryptFinal_ex(aesDecryptCtx, (unsigned char*)*decMsg + decLen, (int*)&blockLen)) {
        return false;
    }
    decLen += blockLen;
 
    EVP_CIPHER_CTX_cleanup(aesDecryptCtx);
    EVP_CIPHER_CTX_free(aesDecryptCtx);
    return (int)decLen;
}

int aesEncrypt(unsigned char *aesKey, unsigned char *aesIV, const unsigned char *msg, size_t msgLen, unsigned char **encMsg) {
    size_t blockLen  = 0;
    size_t encMsgLen = 0;

    EVP_CIPHER_CTX *aesEncryptCtx = (EVP_CIPHER_CTX*)malloc(sizeof(EVP_CIPHER_CTX));
    EVP_CIPHER_CTX_init(aesEncryptCtx);

    *encMsg = (unsigned char*)malloc(msgLen + AES_BLOCK_SIZE);
    if(encMsg == NULL) return false;

    if(!EVP_EncryptInit_ex(aesEncryptCtx, EVP_aes_256_cbc(), NULL, aesKey, aesIV)) {
        return false;
    }

    if(!EVP_EncryptUpdate(aesEncryptCtx, *encMsg, (int*)&blockLen, (unsigned char*)msg, msgLen)) {
        return false;
    }
    encMsgLen += blockLen;

    if(!EVP_EncryptFinal_ex(aesEncryptCtx, *encMsg + encMsgLen, (int*)&blockLen)) {
        return false;
    }

    EVP_CIPHER_CTX_cleanup(aesEncryptCtx);
    EVP_CIPHER_CTX_free(aesEncryptCtx);
    return encMsgLen + blockLen;
}

void getRandIV(unsigned char *ivOut)
{
    RAND_bytes(ivOut, AES_BLOCK_SIZE);
}