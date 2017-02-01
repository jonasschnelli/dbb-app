// Copyright (c) 2015 Jonas Schnelli
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "dbb.h"

#include <assert.h>
#include <cmath>
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>

#include <string>
#include <stdexcept>

#ifndef _SRC_CONFIG__DBB_CONFIG_H
#include "config/_dbb-config.h"
#endif

#include "dbb_util.h"
#include "crypto.h"

#include "univalue.h"
#include "hidapi/hidapi.h"

#include <btc/hash.h>

//defined in libbtc sha2.h
extern "C" {
    extern void hmac_sha512(const uint8_t* key, const uint32_t keylen, const uint8_t* msg, const uint32_t msglen, uint8_t* hmac);
}

#define HID_MAX_BUF_SIZE 5120

#ifdef DBB_ENABLE_DEBUG
#define DBB_DEBUG_INTERNAL(format, args...) printf(format, ##args);
#else
#define DBB_DEBUG_INTERNAL(format, args...)
#endif

namespace DBB
{
static hid_device* HID_HANDLE = NULL;
static enum dbb_device_mode HID_CURRENT_DEVICE_MODE = DBB_DEVICE_UNKNOWN;
static unsigned int readBufSize = HID_REPORT_SIZE_DEFAULT;
static unsigned int writeBufSize = HID_REPORT_SIZE_DEFAULT;
static unsigned char HID_REPORT[HID_MAX_BUF_SIZE] = {0};

#define  USB_REPORT_SIZE 64
#ifndef MIN
#define MIN(a, b)  (((a) < (b)) ? (a) : (b))
#endif


#define HWW_CID 0xff000000

#define TYPE_MASK               0x80    // Frame type mask
#define TYPE_INIT               0x80    // Initial frame identifier
#define TYPE_CONT               0x00    // Continuation frame identifier

#define ERR_INVALID_SEQ         0x04    // Invalid message sequencing

#define U2FHID_ERROR        (TYPE_INIT | 0x3f)  // Error response
#define U2FHID_VENDOR_FIRST (TYPE_INIT | 0x40)  // First vendor defined command
#define HWW_COMMAND         (U2FHID_VENDOR_FIRST + 0x01)// Hardware wallet command

#define FRAME_TYPE(f) ((f).type & TYPE_MASK)
#define FRAME_CMD(f)  ((f).init.cmd & ~TYPE_MASK)
#define MSG_LEN(f)    (((f).init.bcnth << 8) + (f).init.bcntl)
#define FRAME_SEQ(f)  ((f).cont.seq & ~TYPE_MASK)

__extension__ typedef struct {
    uint32_t cid;               // Channel identifier
    union {
        uint8_t type;           // Frame type - bit 7 defines type
        struct {
            uint8_t cmd;        // Command - bit 7 set
            uint8_t bcnth;      // Message byte count - high
            uint8_t bcntl;      // Message byte count - low
            uint8_t data[USB_REPORT_SIZE - 7]; // Data payload
        } init;
        struct {
            uint8_t seq;        // Sequence number - bit 7 cleared
            uint8_t data[USB_REPORT_SIZE - 5]; // Data payload
        } cont;
    };
} USB_FRAME;

static int api_hid_send_frame(USB_FRAME *f)
{
    int res = 0;
    uint8_t d[sizeof(USB_FRAME) + 1];
    memset(d, 0, sizeof(d));
    d[0] = 0;  // un-numbered report
    f->cid = htonl(f->cid);  // cid is in network order on the wire
    memcpy(d + 1, f, sizeof(USB_FRAME));
    f->cid = ntohl(f->cid);

    res = hid_write(HID_HANDLE, d, sizeof(d));

    if (res == sizeof(d)) {
        return 0;
    }
    return 1;
}


static int api_hid_send_frames(uint32_t cid, uint8_t cmd, const void *data, size_t size)
{
    USB_FRAME frame;
    int res;
    size_t frameLen;
    uint8_t seq = 0;
    const uint8_t *pData = (const uint8_t *) data;

    frame.cid = cid;
    frame.init.cmd = TYPE_INIT | cmd;
    frame.init.bcnth = (size >> 8) & 255;
    frame.init.bcntl = (size & 255);

    frameLen = MIN(size, sizeof(frame.init.data));
    memset(frame.init.data, 0xEE, sizeof(frame.init.data));
    memcpy(frame.init.data, pData, frameLen);

    do {
        res = api_hid_send_frame(&frame);
        if (res != 0) {
            return res;
        }

        size -= frameLen;
        pData += frameLen;

        frame.cont.seq = seq++;
        frameLen = MIN(size, sizeof(frame.cont.data));
        memset(frame.cont.data, 0xEE, sizeof(frame.cont.data));
        memcpy(frame.cont.data, pData, frameLen);
    } while (size);

    return 0;
}


static int api_hid_read_frame(USB_FRAME *r)
{

    memset((int8_t *)r, 0xEE, sizeof(USB_FRAME));

    int res = 0;
    res = hid_read(HID_HANDLE, (uint8_t *) r, sizeof(USB_FRAME));

    if (res == sizeof(USB_FRAME)) {
        r->cid = ntohl(r->cid);
        return 0;
    }
    return 1;
}


static int api_hid_read_frames(uint32_t cid, uint8_t cmd, void *data, int max)
{
    USB_FRAME frame;
    int res, result;
    size_t totalLen, frameLen;
    uint8_t seq = 0;
    uint8_t *pData = (uint8_t *) data;

    (void) cmd;

    do {
        res = api_hid_read_frame(&frame);
        if (res != 0) {
            return res;
        }

    } while (frame.cid != cid || FRAME_TYPE(frame) != TYPE_INIT);

    if (frame.init.cmd == U2FHID_ERROR) {
        return -frame.init.data[0];
    }

    totalLen = MIN(max, MSG_LEN(frame));
    frameLen = MIN(sizeof(frame.init.data), totalLen);

    result = totalLen;

    memcpy(pData, frame.init.data, frameLen);
    totalLen -= frameLen;
    pData += frameLen;

    while (totalLen) {
        res = api_hid_read_frame(&frame);
        if (res != 0) {
            return res;
        }

        if (frame.cid != cid) {
            continue;
        }
        if (FRAME_TYPE(frame) != TYPE_CONT) {
            return -ERR_INVALID_SEQ;
        }
        if (FRAME_SEQ(frame) != seq++) {
            return -ERR_INVALID_SEQ;
        }

        frameLen = MIN(sizeof(frame.cont.data), totalLen);

        memcpy(pData, frame.cont.data, frameLen);
        totalLen -= frameLen;
        pData += frameLen;
    }

    return result;
}

static bool api_hid_init(unsigned int writeBufSizeIn = HID_REPORT_SIZE_DEFAULT, unsigned int readBufSizeIn = HID_REPORT_SIZE_DEFAULT)
{
    readBufSize = readBufSizeIn;
    writeBufSize = writeBufSizeIn;

    //TODO: way to handle multiple DBB
    HID_HANDLE = hid_open(0x03eb, 0x2402, NULL); //vendor-id, product-id
    if (!HID_HANDLE) {
        return false;
    }
    return true;
}

static bool api_hid_close(void)
{
    //TODO: way to handle multiple DBB
    if (HID_HANDLE) {
        hid_close(HID_HANDLE); //vendor-id, product-id
        hid_exit();
        return true;
    }

    return false;
}

enum dbb_device_mode deviceAvailable()
{
    struct hid_device_info* devs, *cur_dev;

    devs = hid_enumerate(0x03eb, 0x2402);

    cur_dev = devs;
    enum dbb_device_mode foundType = DBB_DEVICE_NO_DEVICE;
    while (cur_dev) {
        // get the manufacturer wide string
        if (!cur_dev || !cur_dev->manufacturer_string || !cur_dev->serial_number)
        {
            cur_dev = cur_dev->next;
            foundType = DBB_DEVICE_UNKNOWN;
            continue;
        }


        std::wstring wsMF(cur_dev->manufacturer_string);
        std::string strMF( wsMF.begin(), wsMF.end() );

        // get the setial number wide string
        std::wstring wsSN(cur_dev->serial_number);
        std::string strSN( wsSN.begin(), wsSN.end() );

        std::vector<std::string> vSNParts = DBB::split(strSN, ':');

        if ((vSNParts.size() == 2 && vSNParts[0] == "dbb.fw") || strSN == "firmware")
        {
            foundType = DBB_DEVICE_MODE_FIRMWARE;
            // for now, only support one digit version numbers
            if (vSNParts[1].size() >= 6 && vSNParts[1][0] == 'v')
            {
                int major = vSNParts[1][1] - '0';
                int minor = vSNParts[1][3] - '0';
                int patch = vSNParts[1][5] - '0';

                // if version is greater or equal to then 2.1.0, use U2F protocol
                if (major > 2 || (major == 2 && minor >= 1))
                {
                    foundType = DBB_DEVICE_MODE_FIRMWARE_U2F;
                }
            }
            if (vSNParts[1].size() > 2 && vSNParts[1][vSNParts[1].size()-2] == '-' && vSNParts[1][vSNParts[1].size()-1] == '-') {
                foundType = (foundType == DBB_DEVICE_MODE_FIRMWARE) ? DBB_DEVICE_MODE_FIRMWARE_NO_PASSWORD : DBB_DEVICE_MODE_FIRMWARE_U2F_NO_PASSWORD;
            }
            break;
        }
        else if (vSNParts.size() == 2 && vSNParts[0] == "dbb.bl")
        {
            foundType = DBB_DEVICE_MODE_BOOTLOADER;
            break;
        }
        else
        {
            cur_dev = cur_dev->next;
        }
    }
    hid_free_enumeration(devs);
    
    return foundType;
}

bool isConnectionOpen()
{
    return (HID_HANDLE != NULL);
}

bool openConnection(enum dbb_device_mode mode)
{
    if (mode == DBB_DEVICE_MODE_BOOTLOADER && api_hid_init(HID_BL_BUF_SIZE_W, HID_BL_BUF_SIZE_R)) {
        HID_CURRENT_DEVICE_MODE = DBB_DEVICE_MODE_BOOTLOADER;
        return true;
    }
    else if (mode == DBB_DEVICE_MODE_FIRMWARE_U2F && api_hid_init(HID_BL_BUF_SIZE_W, HID_BL_BUF_SIZE_R)) {
        HID_CURRENT_DEVICE_MODE = DBB_DEVICE_MODE_FIRMWARE_U2F;
        return true;
    }
    else
    {
        HID_CURRENT_DEVICE_MODE = DBB_DEVICE_MODE_FIRMWARE;
        return api_hid_init();
    }
}

bool closeConnection()
{
    return api_hid_close();
}

bool sendCommand(const std::string& json, std::string& resultOut)
{
    int res, cnt = 0;

    if (!HID_HANDLE)
        return false;

    DBB_DEBUG_INTERNAL("Sending command: %s\n", json.c_str());

    memset(HID_REPORT, 0, HID_MAX_BUF_SIZE);
    if (json.size()+1 > HID_MAX_BUF_SIZE)
    {
        DBB_DEBUG_INTERNAL("Buffer to small for string to send");
        return false;
    }

    int reportShift = 0;
#ifdef DBB_ENABLE_HID_REPORT_SHIFT
    reportShift = 1;
#endif
    HID_REPORT[0] = 0x00;
    memcpy(HID_REPORT+reportShift, json.c_str(), std::min(HID_MAX_BUF_SIZE, (int)json.size()));
    if (HID_CURRENT_DEVICE_MODE == DBB_DEVICE_MODE_FIRMWARE_U2F)
    {
        int res = api_hid_send_frames(HWW_CID, HWW_COMMAND, json.c_str(), json.size());

        memset(HID_REPORT, 0, HID_MAX_BUF_SIZE);
        res = api_hid_read_frames(HWW_CID, HWW_COMMAND, HID_REPORT, HID_REPORT_SIZE_DEFAULT);
    }
    else {
        if(hid_write(HID_HANDLE, (unsigned char*)HID_REPORT, writeBufSize+reportShift) == -1)
        {
            const wchar_t *error = hid_error(HID_HANDLE);
            if (error)
            {
                std::wstring wsER(error);
                std::string strER( wsER.begin(), wsER.end() );

                DBB_DEBUG_INTERNAL("Error writing to the usb device: %s\n", strER.c_str());
            }
            return false;
        }

        DBB_DEBUG_INTERNAL("try to read some bytes...\n");
        memset(HID_REPORT, 0, HID_MAX_BUF_SIZE);
        while (cnt < readBufSize) {
            res = hid_read(HID_HANDLE, HID_REPORT + cnt, readBufSize);
            if (res < 0 || (res == 0 && cnt < readBufSize)) {
                std::string errorStr = "";
                const wchar_t *error = hid_error(HID_HANDLE);

                if (error)
                {
                    std::wstring wsER(error);
                    errorStr.assign( wsER.begin(), wsER.end() );
                }

                DBB_DEBUG_INTERNAL("HID Read failed or timed out: %s\n", errorStr.c_str());
                return false;
            }
            cnt += res;
        }

        DBB_DEBUG_INTERNAL(" OK, read %d bytes (%s).\n", res, (const char*)HID_REPORT);
    }
    
    resultOut.assign((const char*)HID_REPORT);
    return true;
}

bool sendChunk(unsigned int chunknum, const std::vector<unsigned char>& data, std::string& resultOut)
{
    int res, cnt = 0;

    if (!HID_HANDLE)
        return false;

    DBB_DEBUG_INTERNAL("Sending chunk: %d\n", chunknum);

    assert(data.size() <= HID_MAX_BUF_SIZE-2);
    memset(HID_REPORT, 0xFF, HID_MAX_BUF_SIZE);
    int reportShift = 0;
#ifdef DBB_ENABLE_HID_REPORT_SHIFT
    reportShift = 1;
    HID_REPORT[0] = 0x00;
#endif
    HID_REPORT[0+reportShift] = 0x77;
    HID_REPORT[1+reportShift] = chunknum % 0xff;
    memcpy((void *)&HID_REPORT[2+reportShift], (unsigned char*)&data[0], data.size());

    if(hid_write(HID_HANDLE, (unsigned char*)HID_REPORT, writeBufSize+reportShift) == -1)
    {
        const wchar_t *error = hid_error(HID_HANDLE);
        if (error)
        {
            std::wstring wsER(error);
            std::string strER( wsER.begin(), wsER.end() );

            DBB_DEBUG_INTERNAL("Error writing to the usb device: %s\n", strER.c_str());
        }
        return false;
    }

    DBB_DEBUG_INTERNAL("try to read some bytes...\n");
    memset(HID_REPORT, 0, HID_MAX_BUF_SIZE);
    while (cnt < readBufSize) {
        res = hid_read(HID_HANDLE, HID_REPORT + cnt, readBufSize);
        if (res < 0) {
            throw std::runtime_error("Error: Unable to read HID(USB) report.\n");
        }
        cnt += res;
    }

    DBB_DEBUG_INTERNAL(" OK, read %d bytes.\n", res);
    
    resultOut.assign((const char*)HID_REPORT);
    return true;
}

bool upgradeFirmware(const std::vector<char>& firmwarePadded, size_t firmwareSize, const std::string& sigCmpStr, std::function<void(const std::string&, float progress)> progressCallback)
{
    std::string cmdOut;
    sendCommand("v0", cmdOut);
    if (cmdOut.size() != 1 || cmdOut[0] != 'v')
        return false;
    sendCommand("s0"+sigCmpStr, cmdOut);
    sendCommand("e", cmdOut);

    int cnt = 0;
    size_t pos = 0;
    int nChunks = ceil(firmwareSize / (float)FIRMWARE_CHUNKSIZE);
    progressCallback("", 0.0);
    while (pos+FIRMWARE_CHUNKSIZE < firmwarePadded.size())
    {
        std::vector<unsigned char> chunk(firmwarePadded.begin()+pos, firmwarePadded.begin()+pos+FIRMWARE_CHUNKSIZE);
        DBB::sendChunk(cnt,chunk,cmdOut);
        progressCallback("", 1.0/nChunks*cnt);
        pos += FIRMWARE_CHUNKSIZE;
        if (cmdOut != "w0")
            return false;

        if (pos >= firmwareSize)
            break;
        cnt++;
    }

    sendCommand("s0"+sigCmpStr, cmdOut);
    if (cmdOut.size() < 2)
        return false;
    if (!(cmdOut[0] == 's' && cmdOut[1] == '0'))
        return false;

    progressCallback("", 1.0);

    return true;
}

bool decryptAndDecodeCommand(const std::string& cmdIn, const std::string& password, std::string& stringOut, bool stretch)
{
    unsigned char passwordSha256[BTC_HASH_LENGTH];
    unsigned char aesIV[DBB_AES_BLOCKSIZE];
    unsigned char aesKey[DBB_AES_KEYSIZE];

    if (stretch)
        btc_hash((const uint8_t *)password.c_str(), password.size(), passwordSha256);
    else
        memcpy(passwordSha256, password.c_str(), password.size());

    memcpy(aesKey, passwordSha256, DBB_AES_KEYSIZE);

    std::string textToDecodeAndDecrypt;
    if (stretch)
    {
        UniValue valRead(UniValue::VSTR);
        if (!valRead.read(cmdIn))
            throw std::runtime_error("failed deserializing json");

        UniValue input = find_value(valRead, "input");
        if (!input.isNull() && input.isObject()) {
            UniValue error = find_value(input, "error");
            if (!error.isNull() && error.isStr())
                throw std::runtime_error("Error decrypting: " + error.get_str());
        }

        UniValue ctext = find_value(valRead, "ciphertext");
        if (ctext.isNull())
            throw std::runtime_error("failed deserializing json");

        textToDecodeAndDecrypt = ctext.get_str();
    }
    else
        textToDecodeAndDecrypt = cmdIn;

    std::string base64dec = base64_decode(textToDecodeAndDecrypt);
    unsigned int base64_len = base64dec.size();

    if (base64dec.empty() || (base64_len <= DBB_AES_BLOCKSIZE))
        return false;

    unsigned char* base64dec_c = (unsigned char*)base64dec.c_str();

    unsigned char decryptedStream[base64_len - DBB_AES_BLOCKSIZE];
    unsigned char* decryptedCommand;
    memcpy(aesIV, base64dec_c, DBB_AES_BLOCKSIZE); //copy first 16 bytes and take as IV
    aesDecrypt(aesKey, aesIV, base64dec_c + DBB_AES_BLOCKSIZE, base64_len - DBB_AES_BLOCKSIZE, decryptedStream);

    int decrypt_len = 0;
    int padlen = decryptedStream[base64_len - DBB_AES_BLOCKSIZE - 1];

    if (base64_len <= DBB_AES_BLOCKSIZE + padlen)
        return false;

    char* dec = (char*)malloc(base64_len - DBB_AES_BLOCKSIZE - padlen + 1); // +1 for null termination
    if (!dec) {
        decrypt_len = 0;;
        memset(decryptedStream, 0, sizeof(decryptedStream));
        throw std::runtime_error("decription failed");
        return false;
    }
    int totalLength = (base64_len - DBB_AES_BLOCKSIZE - padlen);
    if (totalLength  < 0 || totalLength > sizeof(decryptedStream) )
    {
        free(dec);
        throw std::runtime_error("decription failed");
        return false;
    }

    memcpy(dec, decryptedStream, base64_len - DBB_AES_BLOCKSIZE - padlen);
    dec[base64_len - DBB_AES_BLOCKSIZE - padlen] = 0;
    decrypt_len = base64_len - DBB_AES_BLOCKSIZE - padlen + 1;

    stringOut.assign((const char*)dec);

    memset(decryptedStream, 0, sizeof(decryptedStream));
    free(dec);
    return true;
}

bool encryptAndEncodeCommand(const std::string& cmd, const std::string& password, std::string& base64strOut, bool stretch)
{
    if (password.empty())
        return false;

    //double sha256 the password
    unsigned char passwordSha256[BTC_HASH_LENGTH];

    unsigned char aesIV[DBB_AES_BLOCKSIZE];
    unsigned char aesKey[DBB_AES_KEYSIZE];

    if (stretch)
        btc_hash((const uint8_t *)password.c_str(), password.size(), passwordSha256);
    else
        memcpy(passwordSha256, password.c_str(), password.size());

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
    for (pads = 0; pads < DBB_AES_BLOCKSIZE - inlen % DBB_AES_BLOCKSIZE; pads++) {
        inpad[inlen + pads] = (DBB_AES_BLOCKSIZE - inlen % DBB_AES_BLOCKSIZE);
    }

    //add iv to the stream for base64 encoding
    memcpy(enc_cat, aesIV, DBB_AES_BLOCKSIZE);

    //encrypt
    unsigned char cypher[inpadlen];
    aesEncrypt(aesKey, aesIV, inpad, inpadlen, cypher);

    //copy the encypted data to the stream where the iv is already
    memcpy(enc_cat + DBB_AES_BLOCKSIZE, cypher, inpadlen);

    //base64 encode
    base64strOut = base64_encode(enc_cat, inpadlen + DBB_AES_BLOCKSIZE);

    return true;
}

void pbkdf2_hmac_sha512(const uint8_t *pass, int passlen, uint8_t *key, int keylen)
{
    uint32_t i, j, k;
    uint8_t f[BACKUP_KEY_PBKDF2_HMACLEN], g[BACKUP_KEY_PBKDF2_HMACLEN];
    uint32_t blocks = keylen / BACKUP_KEY_PBKDF2_HMACLEN;

    static uint8_t salt[BACKUP_KEY_PBKDF2_SALTLEN + 4];
    memset(salt, 0, sizeof(salt));
    memcpy(salt, BACKUP_KEY_PBKDF2_SALT, strlen(BACKUP_KEY_PBKDF2_SALT));

    if (keylen & (BACKUP_KEY_PBKDF2_HMACLEN - 1)) {
        blocks++;
    }
    for (i = 1; i <= blocks; i++) {
        salt[BACKUP_KEY_PBKDF2_SALTLEN    ] = (i >> 24) & 0xFF;
        salt[BACKUP_KEY_PBKDF2_SALTLEN + 1] = (i >> 16) & 0xFF;
        salt[BACKUP_KEY_PBKDF2_SALTLEN + 2] = (i >> 8) & 0xFF;
        salt[BACKUP_KEY_PBKDF2_SALTLEN + 3] = i & 0xFF;
        hmac_sha512(pass, passlen, salt, BACKUP_KEY_PBKDF2_SALTLEN + 4, g);
        memcpy(f, g, BACKUP_KEY_PBKDF2_HMACLEN);
        for (j = 1; j < BACKUP_KEY_PBKDF2_ROUNDS; j++) {
            hmac_sha512(pass, passlen, g, BACKUP_KEY_PBKDF2_HMACLEN, g);
            for (k = 0; k < BACKUP_KEY_PBKDF2_HMACLEN; k++) {
                f[k] ^= g[k];
            }
        }
        if (i == blocks && (keylen & (BACKUP_KEY_PBKDF2_HMACLEN - 1))) {
            memcpy(key + BACKUP_KEY_PBKDF2_HMACLEN * (i - 1), f, keylen & (BACKUP_KEY_PBKDF2_HMACLEN - 1));
        } else {
            memcpy(key + BACKUP_KEY_PBKDF2_HMACLEN * (i - 1), f, BACKUP_KEY_PBKDF2_HMACLEN);
        }
    }
    memset(f, 0, sizeof(f));
    memset(g, 0, sizeof(g));
}

std::string getStretchedBackupHexKey(const std::string &passphrase)
{

    assert(passphrase.size() > 0);

    uint8_t key[BACKUP_KEY_PBKDF2_HMACLEN];
    pbkdf2_hmac_sha512((const uint8_t *)&passphrase[0], passphrase.size(), key, sizeof(key));
    return DBB::HexStr(key, key+sizeof(key));
}
}
