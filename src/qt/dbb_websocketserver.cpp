// Copyright (c) 2015 Jonas Schnelli
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "dbb_websocketserver.h"

WebsocketServer::WebsocketServer(quint16 port, bool debug, QObject *parent) :
QObject(parent),
m_pWebSocketServer(new QWebSocketServer(QStringLiteral("echo"),
                                        QWebSocketServer::NonSecureMode, this)),
m_clients(),
m_debug(debug),
clientInECDHpairing(0)
{
    if (m_pWebSocketServer->listen(QHostAddress::Any, port)) {
        if (m_debug)
            qDebug() << "Echoserver listening on port" << port << "URL: ";
        connect(m_pWebSocketServer, &QWebSocketServer::newConnection,
                this, &WebsocketServer::onNewConnection);
        connect(m_pWebSocketServer, &QWebSocketServer::closed, this, &WebsocketServer::close);
    }
}

WebsocketServer::~WebsocketServer()
{
    m_pWebSocketServer->close();
    qDeleteAll(m_clients.begin(), m_clients.end());
}

void WebsocketServer::close()
{
    int i = 100;
}

void WebsocketServer::onNewConnection()
{
    QWebSocket *pSocket = m_pWebSocketServer->nextPendingConnection();

    connect(pSocket, &QWebSocket::textMessageReceived, this, &WebsocketServer::processTextMessage);
    connect(pSocket, &QWebSocket::binaryMessageReceived, this, &WebsocketServer::processBinaryMessage);
    connect(pSocket, &QWebSocket::disconnected, this, &WebsocketServer::socketDisconnected);

    m_clients << pSocket;

    emit amountOfConnectionsChanged(m_clients.count());
}

void WebsocketServer::sendDataToClientInECDHParingState(const UniValue &data)
{
    if (clientInECDHpairing)
    {
        std::string dataStr = data.write();
        clientInECDHpairing->sendTextMessage(QString::fromStdString(dataStr));
    }
}

int WebsocketServer::sendStringToAllClients(const std::string &data)
{
    for( int i=0; i<m_clients.count(); ++i )
    {
        QWebSocket *pClient = m_clients[i];
        pClient->sendTextMessage(QString::fromStdString(data));
    }
    return m_clients.count();
}

void WebsocketServer::abortECDHPairing()
{
    if (clientInECDHpairing)
    {
        clientInECDHpairing = NULL;
    }
}

void WebsocketServer::processTextMessage(QString message)
{
    QWebSocket *pClient = qobject_cast<QWebSocket *>(sender());

    UniValue responseUni;
    responseUni.read(message.toStdString());
    if (responseUni.isObject())
    {
        UniValue cmdUni;
        cmdUni = find_value(responseUni, "ecdh");

        if (cmdUni.isStr())
        {
            clientInECDHpairing = pClient;
            emit ecdhPairingRequest(cmdUni.get_str());
        }
    }

    if (m_debug)
        qDebug() << "Message received:" << message;
}

void WebsocketServer::processBinaryMessage(QByteArray message)
{
    QWebSocket *pClient = qobject_cast<QWebSocket *>(sender());
    if (m_debug)
        qDebug() << "Binary Message received:" << message;
    if (pClient) {
        pClient->sendBinaryMessage(message);
    }
}

void WebsocketServer::socketDisconnected()
{
    QWebSocket *pClient = qobject_cast<QWebSocket *>(sender());
    if (clientInECDHpairing && clientInECDHpairing == pClient)
    {
        clientInECDHpairing = NULL;
        emit ecdhPairingRequestAbort();
    }

    if (m_debug)
        qDebug() << "socketDisconnected:" << pClient;
    if (pClient) {
        m_clients.removeAll(pClient);
        pClient->deleteLater();
        emit amountOfConnectionsChanged(m_clients.count());
    }
}