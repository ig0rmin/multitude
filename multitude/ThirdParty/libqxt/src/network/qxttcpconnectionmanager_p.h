
/****************************************************************************
** Copyright (c) 2006 - 2011, the LibQxt project.
** See the Qxt AUTHORS file for a list of authors and copyright holders.
** All rights reserved.
**
** Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are met:
**     * Redistributions of source code must retain the above copyright
**       notice, this list of conditions and the following disclaimer.
**     * Redistributions in binary form must reproduce the above copyright
**       notice, this list of conditions and the following disclaimer in the
**       documentation and/or other materials provided with the distribution.
**     * Neither the name of the LibQxt project nor the
**       names of its contributors may be used to endorse or promote products
**       derived from this software without specific prior written permission.
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
** ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
** WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
** DISCLAIMED. IN NO EVENT SHALL <COPYRIGHT HOLDER> BE LIABLE FOR ANY
** DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
** (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
** LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
** ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
** SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
**
** <http://libqxt.org>  <foundation@libqxt.org>
*****************************************************************************/

#ifndef QXTTCPCONNECTIONMANAGER_P_H
#define QXTTCPCONNECTIONMANAGER_P_H

#include <qxttcpconnectionmanager.h>
#include <QTcpServer>
#include <QTcpSocket>
#include <QSignalMapper>

#if !QT_CONFIG(ssl)
class QxtTcpConnectionManagerPrivate : public QTcpServer, public QxtPrivate<QxtTcpConnectionManager>
#else
#include "qxtsslserver.h"
#include <QSslSocket>
class QxtTcpConnectionManagerPrivate : public QxtSslServer, public QxtPrivate<QxtTcpConnectionManager>
#endif
{
Q_OBJECT
friend class QxtSslConnectionManager;
public:
    QxtTcpConnectionManagerPrivate();
    QXT_DECLARE_PUBLIC(QxtTcpConnectionManager)

protected:
#if QT_VERSION >= 0x050000
	void incomingConnection(qintptr socketDescriptor);
#else
    void incomingConnection(int socketDescriptor);
#endif

private Q_SLOTS:
    void socketDisconnected(QObject* client);

private:
    QSignalMapper mapper;
};

#endif
