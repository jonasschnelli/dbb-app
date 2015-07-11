// Copyright (c) 2015 Jonas Schnelli
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "util.h"

#define fPrintToConsole true
int LogPrintStr(const std::string &str)
{
    int ret = 0; // Returns total number of characters written
    if (fPrintToConsole)
    {
        // print to console
        ret = fwrite(str.data(), 1, str.size(), stdout);
        fflush(stdout);
    }

    return ret;
}

std::string SanitizeString(const std::string& str)
{
    /**
     * safeChars chosen to allow simple messages/URLs/email addresses, but avoid anything
     * even possibly remotely dangerous like & or >
     */
    static std::string safeChars("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ01234567890 .,;_/:?@()");
    std::string strResult;
    for (std::string::size_type i = 0; i < str.size(); i++)
    {
        if (safeChars.find(str[i]) != std::string::npos)
            strResult.push_back(str[i]);
    }
    return strResult;
}