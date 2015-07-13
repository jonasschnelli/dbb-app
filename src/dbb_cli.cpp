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

#include <assert.h>
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>

#include <string>

#include "../include/dbb.h"
#include "util.h"

#include "../include/univalue.h"
#include "hidapi/hidapi.h"
#include "openssl/sha.h"

//simple dispatch class for a dbb command
class CDBBCommand
{
public:
    std::string cmdname;
    std::string json;
    bool requiresEncryption;
};

static const CDBBCommand vCommands[] =
{
    { "erase"           , "{\"reset\" : \"__ERASE__\"}",                            false},
    { "password"        , "{\"password\" : \"0000\"}",                              false},
    { "led"             , "{\"led\" : \"toggle\"}",                                 true},
    { "seed"            , "{\"seed\" : {\"source\" : \"create\"} }",                true},
    //TODO add all missing commands
    //parameters could be added with something like "{\"seed\" : {\"source\" : \"$1\"} }" (where $1 will be replace with argv[1])
};

int main( int argc, char *argv[] )
{
    if (!DBB::openConnection())
        printf("Error: No digital bitbox connected\n");
    
    else {
        DebugOut("main", "Digital Bitbox Connected\n");

        if (argc < 2)
        {
            printf("no command given\n");
            return 0;
        }
        unsigned int i = 0, loop = 0;
        bool cmdfound = false;
        std::string userCmd = std::string(argv[1]);
        
        for (loop = 0; loop < 100; loop++)
        {
        for (i = 0; i < (sizeof(vCommands) / sizeof(vCommands[0])); i++)
        {
            CDBBCommand cmd = vCommands[i];
            if (cmd.cmdname == userCmd)
            {
                std::string password = "0000";
                std::string cmdOut;

                if (cmd.requiresEncryption)
                {
                    std::string base64str;
                    DBB::encryptAndEncodeCommand(cmd.json, password, base64str);
                    
                    DBB::sendCommand(base64str, cmdOut);
                    
                    std::string unencryptedJson;
                    DBB::decryptAndDecodeCommand(cmdOut, password, unencryptedJson);
                    
                    //example json en/decode
                    UniValue json;
                    json.read(unencryptedJson);
                    std::string jsonFlat = json.write(2);
                    printf("result: %s\n", jsonFlat.c_str());
                }
                else
                {
                    //send command unencrypted
        	        DBB::sendCommand(cmd.json, cmdOut);
                    printf("result: %s\n", cmdOut.c_str());
                }
                cmdfound = true;
            }
        }
        }
        if (!cmdfound)
        {
            //try to send it as raw json
            if (userCmd.size() > 1 && userCmd.at(0) == '{') //todo: ignore whitespace
            {
                std::string cmdOut;
                DebugOut("main", "Send raw json %s\n", userCmd.c_str());
        	    cmdfound = DBB::sendCommand(userCmd, cmdOut);
            }

            printf("command (%s) not found\n", SanitizeString(userCmd).c_str());
        }
    }
    return 0;
}
