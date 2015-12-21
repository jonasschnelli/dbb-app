// Copyright (c) 2015 Jonas Schnelli
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef DBB_WEBSOCKET_H
#define DBB_WEBSOCKET_H

#include <QList>

#include "QtWebSockets/qwebsocketserver.h"
#include "QtWebSockets/qwebsocket.h"

#include <univalue.h>

class WebsocketServer : public QObject
{
    Q_OBJECT
public:
    explicit WebsocketServer(quint16 port, bool debug = false, QObject *parent = Q_NULLPTR);
    ~WebsocketServer();

    int sendStringToAllClients(const std::string &data);
    void sendDataToClientInECDHParingState(const UniValue &data);
    void abortECDHPairing();
    
Q_SIGNALS:
    void closed();
    void ecdhPairingRequest(const std::string &pubkey);
    void ecdhPairingRequestAbort();
    void amountOfConnectionsChanged(int);
    
private Q_SLOTS:
    void onNewConnection();
    void socketDisconnected();
    void close();
    void processBinaryMessage(QByteArray message);
    void processTextMessage(QString message);


private:
    QWebSocketServer *m_pWebSocketServer;
    QList<QWebSocket *> m_clients;
    QWebSocket *clientInECDHpairing;
    bool m_debug;
};

#endif
