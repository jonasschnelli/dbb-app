// Copyright (c) 2015 Jonas Schnelli
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef LIBDBB_UTIL_H
#define LIBDBB_UTIL_H

#include <stdint.h>
#include <string>

int LogPrintStr(const std::string &str);
static inline int LogPrint(const char* category, const char* format)
{
    return LogPrintStr(format);
}

//sanitize a string and clean out things which could generate harm over a RPC/JSON/Console output
std::string SanitizeString(const std::string& str);

#endif // LIBDBB_UTIL_H