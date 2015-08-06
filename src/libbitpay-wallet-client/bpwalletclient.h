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

#include <openssl/sha.h>
#include <openssl/err.h>
#include <openssl/rand.h>

#include <secp256k1.h>

#include <stdio.h>
#include <string>

#include <curl/curl.h>

#include <assert.h>
#include <cstring>
#include <stdexcept>
#include <stdint.h>
#include <string>
#include <vector>

#include "uint256.h"
#include "hash.h"
#include "pubkey.h"
#include "random.h"
#include "arith_uint256.h"
#include "crypto/common.h"
#include "crypto/hmac_sha512.h"
#include "eccryptoverify.h"

class ECSignature
{
public:
    secp256k1_ecdsa_signature_t signature;
    
    unsigned int size() const { return sizeof(signature); }
    const unsigned char* begin() const { return signature.data; }
    const unsigned char* end() const { return signature.data + size(); }
};


class ECPubKey
{
private:

    //! Compute the length of a pubkey with a given first byte.
    unsigned int static GetLen(unsigned char chHeader)
    {
        if (chHeader == 2 || chHeader == 3)
            return 33;
        if (chHeader == 4 || chHeader == 6 || chHeader == 7)
            return 65;
        return 0;
    }
    
    //! Comparator implementation.
    friend bool operator==(const ECPubKey& a, const ECPubKey& b)
    {
        return a.pubkey.data[0] == b.pubkey.data[0] &&
               memcmp(a.pubkey.data, b.pubkey.data, a.size()) == 0;
    }
    
public:
    secp256k1_pubkey_t pubkey;
    unsigned int size() const { return sizeof(pubkey.data); }
    const unsigned char* begin() const { return pubkey.data; }
    const unsigned char* end() const { return pubkey.data + size(); }
    bool IsValid() const { return size() > 0; }
    
    bool Verify(const uint256& hash, const std::vector<unsigned char>& vchSig) const;
    std::string GetHex();
    
    bool Verify(const uint256 &hash, const ECSignature &signature);
};

/** An encapsulated private key. */
class ECKey
{
private:
    //! Whether this private key is valid. We check for correctness when modifying the key
    //! data, so fValid should always correspond to the actual state.
    bool fValid;

    //! Whether the public key corresponding to this private key is (to be) compressed.
    bool fCompressed;

    //! The actual byte data
    unsigned char vch[32];
    
    friend bool operator==(const ECKey& a, const ECKey& b)
    {
        return a.fCompressed == b.fCompressed && a.size() == b.size() &&
               memcmp(&a.vch[0], &b.vch[0], a.size()) == 0;
    }
    
public:
    void MakeNewKey(bool fCompressedIn = true);
    ECPubKey GetPubKey() const;
    
    //! Simple read-only vector-like interface.
    unsigned int size() const { return (fValid ? 32 : 0); }
    const unsigned char* begin() const { return vch; }
    const unsigned char* end() const { return vch + size(); }
    
    //! Check whether this private key is valid.
    bool IsValid() const { return fValid; }
    bool Sign(const uint256 &hash, ECSignature &signature) const;
    bool VerifyPubKey() const;
};

const unsigned int BIP32_EXTKEY_SIZE = 74;
typedef uint256 ChainCode;

struct ECExtPubKey {
    unsigned char nDepth;
    unsigned char vchFingerprint[4];
    unsigned int nChild;
    ChainCode chaincode;
    ECPubKey pubkey;

    friend bool operator==(const ECExtPubKey &a, const ECExtPubKey &b)
    {
        return a.nDepth == b.nDepth && memcmp(&a.vchFingerprint[0], &b.vchFingerprint[0], 4) == 0 && a.nChild == b.nChild &&
               a.chaincode == b.chaincode && a.pubkey == b.pubkey;
    }

    void Encode(unsigned char code[BIP32_EXTKEY_SIZE]) const;
    void Decode(const unsigned char code[BIP32_EXTKEY_SIZE]);
    bool Derive(ECExtPubKey& out, unsigned int nChild) const;
};

struct ECExtKey {
    unsigned char nDepth;
    unsigned char vchFingerprint[4];
    unsigned int nChild;
    ChainCode chaincode;
    ECKey key;

    friend bool operator==(const ECExtKey& a, const ECExtKey& b)
    {
        return a.nDepth == b.nDepth && memcmp(&a.vchFingerprint[0], &b.vchFingerprint[0], 4) == 0 && a.nChild == b.nChild &&
               a.chaincode == b.chaincode && a.key == b.key;
    }

    void Encode(unsigned char code[BIP32_EXTKEY_SIZE]) const;
    void Decode(const unsigned char code[BIP32_EXTKEY_SIZE]);
    bool Derive(ECExtKey& out, unsigned int nChild) const;
    ECExtPubKey Neuter() const;
    void SetMaster(const unsigned char* seed, unsigned int nSeedLen);
};

class BitPayWalletClient
{
public:
    BitPayWalletClient();
    ~BitPayWalletClient();
    
    ECKey GetNewKey();
    
private:
    void InitECContext();
};