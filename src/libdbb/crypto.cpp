// Copyright (c) 2015 Jonas Schnelli
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "crypto.h"

#include <btc/aes.h>
#include <btc/random.h>

#include <string.h>

//ignore osx depracation warning
#pragma clang diagnostic ignored "-Wdeprecated-declarations"

void aesDecrypt(unsigned char* aesKey, unsigned char* aesIV, unsigned char* encMsg, size_t encMsgLen, unsigned char* decMsg)
{
    aes_context ctx[1];
    aes_set_key(aesKey, 32, ctx);
    aes_cbc_decrypt(encMsg, decMsg, encMsgLen / N_BLOCK, aesIV, ctx);
    
    memset(ctx, 0, sizeof(ctx)); //clean password
}

void aesEncrypt(unsigned char* aesKey, unsigned char* aesIV, const unsigned char* msg, size_t msgLen, unsigned char* encMsg)
{
    aes_context ctx[1];
    aes_set_key(aesKey, 32, ctx);
    aes_cbc_encrypt(msg, encMsg, msgLen / N_BLOCK, aesIV, ctx);

    memset(ctx, 0, sizeof(ctx)); //clean password
}

void getRandIV(unsigned char* ivOut)
{
    random_init();
    random_bytes(ivOut, N_BLOCK, 0);
}
