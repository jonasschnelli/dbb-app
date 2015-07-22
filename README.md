[![License](http://img.shields.io/:License-MIT-yellow.svg)](LICENSE)

**C++ library for communicating with the [Digital Bitbox](https://digitalbitbox.com) hardware wallet.**

This library includes UniValue – a tiny JSON parser/encoder written by Jeff Garzik (stable and also used in bitcoin-core).

## Example

```cpp
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
```


## dbb-daemon
The dbb daemon can be enabled with the configure option `--enable-daemon`. Optional the gui can be added with `--with-gui=qt5`.


## dbb-cli
This package includes a small tool "dbb-cli" which can be used to direcly talk with your digital bitbox device.


**Examples:**

* `dbb-cli erase`
* `dbb-cli -newpassword=0000 password`
* `dbb-cli -newpassword=test -password=0000 password`
* `dbb-cli -password=test led`
* `dbb-cli -password=test seed`
* `dbb-cli -keypath=m/44/0 xpub`

Available commands with possible arguments (* = mandatory):

```
erase
password -*newpassword
led
seed -source (default: create), -decrypt (default: no), -salt (default: no)
backuplist
backuperase
backup -encrypt (default: no), -filename (default: backup.dat)
sign -type (default: transaction), -data (default: transaction), -keypath (default: transaction), -changekeypath (default: transaction)
xpub -*keypath
name -*name
random -mode (default: true)
sn
version
lock
verifypass -operation (default: create)
aes -type (default: encrypt), -data (default: encrypt)
```

## Current Status
Libdbb is at an early stage of development.

**TODOS**

- Remove openssl requirement by a fallback or full replacement for sha256 and aes256-cbc.
- Extend dbb-cli with all missing commands.
- Add a daemon with support for JSON RPC 2.0 or ZMQ after a potential standard (needs BIPing).

## Build Instructions
Dependencies:

- openssl
- https://github.com/signal11/hidapi
- libevent2 (if daemon enabled)
- qt5 (if UI enabled)

OSX:

    brew install hidapi libevent qt5

Linux (Ubuntu 15.04):

Basics

    sudo apt-get install build-essential libtool autotools-dev autoconf pkg-config git
    sudo apt-get libhidapi-dev

For the daemon

    sudo apt-get libevent-dev

For QT UI

    sudo apt-get install libqt5gui5 libqt5core5a libqt5dbus5 qttools5-dev qttools5-dev-tools

if libhidapi is not available, compile it yourself

    sudo apt-get install libudev-dev libusb-1.0-0-dev lib
    git clone git://github.com/signal11/hidapi.git
    ./bootstrap
    ./configure
    make
    sudo make install



Basic build steps:

    ./autogen
    ./configure --enable-debug --enable-daemon --with-gui=qt5
    make
    sudo make install
