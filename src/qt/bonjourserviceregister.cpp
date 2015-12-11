/*
Copyright (c) 2007, Trenton Schulz

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

 1. Redistributions of source code must retain the above copyright notice,
    this list of conditions and the following disclaimer.

 2. Redistributions in binary form must reproduce the above copyright notice,
    this list of conditions and the following disclaimer in the documentation
    and/or other materials provided with the distribution.

 3. The name of the author may not be used to endorse or promote products
    derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO
EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include "bonjourserviceregister.h"

#ifndef _SRC_CONFIG__DBB_CONFIG_H
#include "config/_dbb-config.h"
#endif

#ifdef DBB_HAVE_DNS_SD_H
#include <dns_sd.h>
#endif

#include <QtCore/QSocketNotifier>

BonjourServiceRegister::BonjourServiceRegister(QObject *parent)
    : QObject(parent), dnssref(0), bonjourSocket(0)
{
}

BonjourServiceRegister::~BonjourServiceRegister()
{
#ifdef DBB_HAVE_DNS_SD_H
    if (dnssref) {
        DNSServiceRefDeallocate(dnssref);
        dnssref = 0;
    }
#endif
}

void BonjourServiceRegister::registerService(const BonjourRecord &record, quint16 servicePort)
{
#ifdef DBB_HAVE_DNS_SD_H
    if (dnssref) {
        qWarning("Warning: Already registered a service for this object, aborting new register");
        return;
    }
    quint16 bigEndianPort = servicePort;
#if Q_BYTE_ORDER == Q_LITTLE_ENDIAN
    {
        bigEndianPort =  0 | ((servicePort & 0x00ff) << 8) | ((servicePort & 0xff00) >> 8);
    }
#endif

    DNSServiceErrorType err = DNSServiceRegister(&dnssref, 0, 0, record.serviceName.toUtf8().constData(),
                                                 record.registeredType.toUtf8().constData(),
                                                 record.replyDomain.isEmpty() ? 0
                                                          : record.replyDomain.toUtf8().constData(), 0,
                                                 bigEndianPort, 0, 0, bonjourRegisterService, this);
    if (err != kDNSServiceErr_NoError) {
        emit error(err);
    } else {
        int sockfd = DNSServiceRefSockFD(dnssref);
        if (sockfd == -1) {
            emit error(kDNSServiceErr_Invalid);
        } else {
            bonjourSocket = new QSocketNotifier(sockfd, QSocketNotifier::Read, this);
            connect(bonjourSocket, SIGNAL(activated(int)), this, SLOT(bonjourSocketReadyRead()));
        }
    }
#endif
}


void BonjourServiceRegister::bonjourSocketReadyRead()
{
#ifdef DBB_HAVE_DNS_SD_H
    DNSServiceErrorType err = DNSServiceProcessResult(dnssref);
    if (err != kDNSServiceErr_NoError)
        emit error(err);
#endif
}


void BonjourServiceRegister::bonjourRegisterService(DNSServiceRef, DNSServiceFlags,
                                                   DNSServiceErrorType errorCode, const char *name,
                                                   const char *regtype, const char *domain,
                                                   void *data)
{
#ifdef DBB_HAVE_DNS_SD_H
    BonjourServiceRegister *serviceRegister = static_cast<BonjourServiceRegister *>(data);
    if (errorCode != kDNSServiceErr_NoError) {
        emit serviceRegister->error(errorCode);
    } else {
        serviceRegister->finalRecord = BonjourRecord(QString::fromUtf8(name),
                                                QString::fromUtf8(regtype),
                                                QString::fromUtf8(domain));
        emit serviceRegister->serviceRegistered(serviceRegister->finalRecord);
    }
#endif
}
