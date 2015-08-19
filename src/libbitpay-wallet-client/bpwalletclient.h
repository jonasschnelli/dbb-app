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

#ifndef BP_WALLET_CLIENT_H
#define BP_WALLET_CLIENT_H

#include <openssl/sha.h>
#include <openssl/err.h>
#include <openssl/rand.h>


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
#include "key.h"
#include "random.h"
#include "univalue/univalue.h"


class BitpayWalletInvitation
{
public:
    std::string walletID;
    CKey walletPrivKey;
    std::string network;
};

class BitPayWalletClient
{
public:
    BitPayWalletClient();
    ~BitPayWalletClient();

    CKey GetNewKey();
    bool ParseWalletInvitation(const std::string& walletInvitation, BitpayWalletInvitation& invitationOut);
    bool GetRequestPubKey(std::string& pubKeyOut);
    std::string GetCopayerId();
    bool GetCopayerHash(const std::string& name, std::string& hashOut);
    bool GetCopayerSignature(const std::string& stringToHash, const CKey& privKey, std::string& sigHexOut);
    void seed();
    bool JoinWallet(const std::string& name, const std::string& walletInvitation, std::string& response);
    bool GetWallets(std::string& response);
    std::string ParseTxProposal(const UniValue& txProposal, std::vector<std::pair<std::string,uint256> >& vInputTxHashes);
    bool PostSignaturesForTxProposal(const UniValue& txProposal, const std::vector<std::string>& vHexSigs);
    bool BroadcastProposal(const UniValue& txProposal);
    
    std::string GetXPubKey();
    std::string SignRequest(const std::string& method,
                            const std::string& url,
                            const std::string& args);
    bool SendRequest(const std::string& method,
                            const std::string& url,
                            const std::string& args,
                            std::string& responseOut,
                            long& httpStatusCodeOut);
                            
    void setPubKeys(const std::string& xPubKeyRequestKeyEntropy, const std::string& xPubKey);

    bool IsSeeded();
    void SaveLocalData();
    void LoadLocalData();
    
    //flip byte order
    static std::string ReversePairs(const std::string& strIn);

private:
    CExtKey masterPrivKey;   // "m/45'"
    CExtPubKey masterPubKey; // "m/45'"
    CKey requestKey;         //"m/1'/0"

    std::string baseURL;

    std::vector<std::string> split(const std::string& str, std::vector<int> indexes);
    std::string _copayerHash(const std::string& name, const std::string& xPubKey, const std::string& requestPubKey);
};
#endif //BP_WALLET_CLIENT_H