// Copyright (c) 2015 Jonas Schnelli
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "bpwalletclient.h"

#include <assert.h>
#include <string.h>

#include "util.h"



//ignore osx depracation warning
#pragma clang diagnostic ignored "-Wdeprecated-declarations"

extern void doubleSha256(char* string, unsigned char* hashOut);
static secp256k1_context_t* secp256k1_context = NULL;

namespace eccrypto {

    /** Order of secp256k1's generator minus 1. */
    const unsigned char vchMaxModOrder[32] = {
        0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
        0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFE,
        0xBA,0xAE,0xDC,0xE6,0xAF,0x48,0xA0,0x3B,
        0xBF,0xD2,0x5E,0x8C,0xD0,0x36,0x41,0x40
    };

    /** Half of the order of secp256k1's generator minus 1. */
    const unsigned char vchMaxModHalfOrder[32] = {
        0x7F,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
        0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
        0x5D,0x57,0x6E,0x73,0x57,0xA4,0x50,0x1D,
        0xDF,0xE9,0x2F,0x46,0x68,0x1B,0x20,0xA0
    };

    const unsigned char vchZero[1] = {0};
    
    int CompareBigEndian(const unsigned char *c1, size_t c1len, const unsigned char *c2, size_t c2len) {
        while (c1len > c2len) {
            if (*c1)
                return 1;
            c1++;
            c1len--;
        }
        while (c2len > c1len) {
            if (*c2)
                return -1;
            c2++;
            c2len--;
        }
        while (c1len > 0) {
            if (*c1 > *c2)
                return 1;
            if (*c2 > *c1)
                return -1;
            c1++;
            c2++;
            c1len--;
        }
        return 0;
    }
    
    bool Check(const unsigned char *vch) {
        return vch &&
               CompareBigEndian(vch, 32, vchZero, 0) > 0 &&
               CompareBigEndian(vch, 32, vchMaxModOrder, 32) <= 0;
    }
}

BitPayWalletClient::BitPayWalletClient()
{
    InitECContext();
}

BitPayWalletClient::~BitPayWalletClient()
{
}

void BitPayWalletClient::InitECContext()
{
    secp256k1_context = secp256k1_context_create(SECP256K1_CONTEXT_SIGN | SECP256K1_CONTEXT_VERIFY);
    unsigned char seed[32];
    RAND_bytes(seed, 32);
    secp256k1_context_randomize(secp256k1_context, seed);
}

ECKey BitPayWalletClient::GetNewKey()
{
    ECKey key;
    key.MakeNewKey();
    return key;
}

void ECKey::MakeNewKey(bool fCompressedIn) {
    do {
        RAND_bytes(vch, sizeof(vch));
    } while (!eccrypto::Check(vch));
    fValid = true;
    fCompressed = fCompressedIn;
}

bool ECKey::Sign(const uint256 &hash, ECSignature &signature) const {
    if (!fValid)
        return false;
    unsigned char extra_entropy[32] = {0};
    int ret = secp256k1_ecdsa_sign(secp256k1_context, hash.begin(), (secp256k1_ecdsa_signature_t *)signature.begin(), begin(), secp256k1_nonce_function_rfc6979, NULL);
    assert(ret);
    return true;
}

ECPubKey ECKey::GetPubKey() const {
    assert(fValid);
    ECPubKey result;
    assert(secp256k1_ec_pubkey_create(secp256k1_context, (secp256k1_pubkey_t *)result.begin(), begin()) == 1);
    int len = result.size();
    assert(result.IsValid());
    return result;
}

bool ECKey::VerifyPubKey() const {
    std::string str = "Bitcoin key verification\n";
    uint256 hashD;
    doubleSha256((char*)str.c_str(), hashD.begin());
    ECSignature sig;
    Sign(hashD, sig);
    return GetPubKey().Verify(hashD, sig);
}



std::string ECPubKey::GetHex() {
    unsigned char pubkeyc[65];
    int pubkeyclen = 65;
    secp256k1_ec_pubkey_serialize(secp256k1_context, pubkeyc, &pubkeyclen, &pubkey, true);
    
    return HexStr(pubkeyc, pubkeyc+pubkeyclen, false);
}

bool ECPubKey::Verify(const uint256 &hash, const ECSignature &signature) {
    unsigned char pubkeyc[65];
    int pubkeyclen = 65;
    return secp256k1_ecdsa_verify(secp256k1_context, hash.begin(), (secp256k1_ecdsa_signature_t *)signature.begin(), &pubkey);
}

