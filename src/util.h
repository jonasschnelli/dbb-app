// Copyright (c) 2015 Jonas Schnelli
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef LIBDBB_UTIL_H
#define LIBDBB_UTIL_H

#include <stdint.h>
#include <string>
#include <map>
#include <vector>

#if defined(HAVE_CONFIG_H)
#include "config/dbb-config.h"
#endif


namespace DBB
{
#define DEBUG_SHOW_CATEGRORY 1

#ifdef ENABLE_DEBUG
#define DebugOut(category, format, args...)  \
    if (DEBUG_SHOW_CATEGRORY) {              \
        printf("  [DEBUG]  %s: ", category); \
    }                                        \
    printf(format, ##args);
#else
#define DebugOut(category, format, args...)
#endif

#define PrintConsole(format, args...) printf(format, ##args);

//sanitize a string and clean out things which could generate harm over a RPC/JSON/Console output
std::string SanitizeString(const std::string& str);

extern std::map<std::string, std::string> mapArgs;
extern std::map<std::string, std::vector<std::string> > mapMultiArgs;
void ParseParameters(int argc, const char* const argv[]);
std::string GetArg(const std::string& strArg, const std::string& strDefault);

std::string HexStr(unsigned char* itbegin, unsigned char* itend, bool fSpaces=false);
std::vector<unsigned char> ParseHex(const char* psz);
std::vector<unsigned char> ParseHex(const std::string& str);
signed char HexDigit(char c);
}
#endif // LIBDBB_UTIL_H