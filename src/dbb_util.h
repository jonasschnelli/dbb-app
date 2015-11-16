// Copyright (c) 2015 Jonas Schnelli
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef LIBDBB_UTIL_H
#define LIBDBB_UTIL_H

#include <stdint.h>
#include <string>
#include <map>
#include <memory>
#include <vector>

#ifndef _SRC_CONFIG__DBB_CONFIG_H
#include "config/_dbb-config.h"
#endif


namespace DBB
{
#define DEBUG_SHOW_CATEGRORY 1

#ifdef DBB_ENABLE_DEBUG
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

std::vector<std::string> &split(const std::string &s, char delim, std::vector<std::string> &elems);
std::vector<std::string> split(const std::string &s, char delim);

template<typename ... Args>
std::string string_format( const std::string& format, Args ... args )
{
    size_t size = std::snprintf( nullptr, 0, format.c_str(), args ... ) + 1; // Extra space for '\0'
    std::unique_ptr<char[]> buf( new char[ size ] );
    std::snprintf( buf.get(), size, format.c_str(), args ... );
    return std::string( buf.get(), buf.get() + size - 1 ); // We don't want the '\0' inside
}
std::string formatMoney(const int64_t &n);
bool ParseMoney(const std::string& str, int64_t& nRet);
bool ParseMoney(const char* pszIn, int64_t& nRet);
}
#endif // LIBDBB_UTIL_H