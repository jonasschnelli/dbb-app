// Copyright (c) 2015 Jonas Schnelli
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef LIBDBB_CRYPTO_H
#define LIBDBB_CRYPTO_H

#include <string>

#define DBB_AES_BLOCKSIZE 16
#define DBB_AES_KEYSIZE 32
#define DBB_SHA256_DIGEST_LENGTH 32

std::string base64_encode(unsigned char const* bytes_to_encode, unsigned int in_len);
std::string base64_decode(std::string const& encoded_string);
int aesDecrypt(unsigned char *aesKey, unsigned char *aesIV, unsigned char *encMsg, size_t encMsgLen, unsigned char **decMsg);
int aesEncrypt(unsigned char *aesKey, unsigned char *aesIV, const unsigned char *msg, size_t msgLen, unsigned char **encMsg);

//generate a two round sha256 hash
void doubleSha256(char *string, unsigned char *hashOut);

//get random aes IV (16 bytes)
void getRandIV(unsigned char *ivOut);

#endif //LIBDBB_CRYPTO_H