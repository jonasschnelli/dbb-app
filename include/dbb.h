// Copyright (c) 2015 Jonas Schnelli
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <stdio.h>
#include <string>

namespace DBB {
//!open a connection to the digital bitbox device
// retruns false if no connection could be made, keeps connection handling
// internal
bool openConnection();

//!close the connection to the dbb device
bool closeConnection();

//!return true if a USBHID connection is open
bool isConnectionOpen();

//!send a json command to the device which is currently open
bool sendCommand(const std::string &json, std::string &resultOut);

//!decrypt a json result
bool decryptAndDecodeCommand(const std::string &cmdIn,
                             const std::string &password,
                             std::string &stringOut);

//!encrypts a json command
bool encryptAndEncodeCommand(const std::string &cmd,
                             const std::string &password,
                             std::string &base64strOut);
}
