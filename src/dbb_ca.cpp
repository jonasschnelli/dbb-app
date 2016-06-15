// Copyright (c) 2016 Jonas Schnelli
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <string>
#include <cstdio>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

namespace DBB
{
static const char *ca_paths[] = {
    "/etc/ssl/certs/ca-certificates.crt", // Debian/Ubuntu/Gentoo etc.
    "/etc/ssl/certs/ca-bundle.crt",       // Debian/Ubuntu/Gentoo etc.
    "/etc/pki/tls/certs/ca-bundle.crt",   // Fedora/RHEL
    "/etc/ssl/ca-bundle.pem",             // OpenSUSE
    "/etc/pki/tls/cacert.pem",            // OpenELEC
};

inline bool file_exists (const char *name) {
    struct stat buffer;
    int result = stat(name, &buffer);
    return (result == 0);
}

std::string getCAFile()
{
    size_t i = 0;
    for( i = 0; i < sizeof(ca_paths) / sizeof(ca_paths[0]); i++)
    {
        if (file_exists(ca_paths[i]))
        {
            return std::string(ca_paths[i]);
        }
    }
    return "";
}
}