[![License](http://img.shields.io/:License-MIT-yellow.svg)](LICENSE)

**C++ library for communicating with the [Digital Bitbox](https://digitalbitbox.com) hardware wallet.**

This library includes UniValue a tiny json en-/decoded written by Jeff Garzik (stable and also used in bitcoin-core).

## Example

    std::string base64str;
    std::string cmdOut;
    std::string password = "0000";
    std::string jsonIn = "{\"led\" : \"toggle\"}";
    DBB::encryptAndEncodeCommand(jsonIn, password, base64str);
    
    DBB::sendCommand(base64str, cmdOut);
    
    std::string decryptedJson;
    DBB::decryptAndDecodeCommand(cmdOut, password, decryptedJson);
    
    //example json en/decode
    UniValue json;
    json.read(decryptedJson);
    std::string jsonFlat = json.write(2);
    printf("result: %s\n", jsonFlat.c_str());

## Build Instructions
Dependencies:

- https://github.com/signal11/hidapi

OSX:

    brew install hidapi

Linux:

    sudo apt-get install libudev-dev libusb-1.0-0-dev
    git clone git://github.com/signal11/hidapi.git
    ./bootstrap
    ./configure
    make
    sudo make install
    


--------------

Basic build steps:

    autoreconf -i -f
    ./configure --enable-debug (--with-hid-libdir= if you like to link to a special path where your libhdi is installe)
    make
    sudo make install