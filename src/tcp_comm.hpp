// Copyright 2017 Hunter L. Allen
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifndef __TCP_COMM_HPP__
#define __TCP_COMM_HPP__
#include <QtNetwork>
#include <QtCore>

class tcp_comm : public QObject
{
public:
  explicit tcp_comm(QString _host = "localhost")
  : m_host(_host)
  {m_p_mutex = new QMutex();}
  virtual ~tcp_comm() {delete m_p_mutex;}

  QString * get_host() {return &m_host;}

  static const unsigned int MAX_RESPONSE = 100 * 1024;
  /* 0.5 second timieout */
  static const unsigned int TIMEOUT = 500;

  void set_host(QString _host)
  {
    m_p_mutex->lock();
    m_host = _host;
    m_p_mutex->unlock();
  }

  bool reconnect(int port)
  {
    m_p_mutex->lock();
    QTcpSocket * pSocket = new QTcpSocket();
    pSocket->connectToHost(m_host, port, QIODevice::ReadWrite);
    bool toReturn = pSocket->waitForConnected(TIMEOUT);
    pSocket->abort();
    delete pSocket;
    m_p_mutex->unlock();
    return toReturn;
  }

  int send_text(int port, char * command, char * response)
  {
    m_p_mutex->lock();
    QTcpSocket * pSocket = new QTcpSocket();

    QString copy = m_host;
    pSocket->connectToHost(copy, port, QIODevice::ReadWrite);
    pSocket->waitForConnected(TIMEOUT);
    if (pSocket->state() == QAbstractSocket::UnconnectedState) {
      m_p_mutex->unlock();
      pSocket->abort();
      delete pSocket;
      return 0;
    }

    pSocket->waitForReadyRead(-1);
    QString iHeard;

    while (pSocket->canReadLine()) {
      iHeard += QString(pSocket->readLine());
    }

    QString toSend(command);
    toSend += "\r\n";
    pSocket->write(toSend.toUtf8());
    pSocket->waitForBytesWritten(-1);
    pSocket->waitForReadyRead(-1);
    while (pSocket->canReadLine()) {
      iHeard += QString(pSocket->readLine());
    }
    memcpy(response, iHeard.toStdString().c_str(), iHeard.size());
    m_p_mutex->unlock();
    pSocket->abort();
    delete pSocket;
    return 1;
  }

private:
  QString m_host;
  QMutex * m_p_mutex;
};//end class
#endif
