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

#ifndef __TCP_THREAD_HPP__
#define __TCP_THREAD_HPP__

#include <iostream>

#include <QDataStream>
#include <QtNetwork>
#include <QtCore>
#include <QDebug>

#include "worker_connection.hpp"
#include "client_connection.hpp"
#include "master_node.hpp"
#include "worker_node.hpp"

class master_node;
class worker_node;

class tcp_thread : public QObject
{
  Q_OBJECT

public:
  explicit tcp_thread(
    const QString & _hostname, const quint16 & _port,
    const bool & _master_mode = true, QObject * parent = NULL);
  virtual ~tcp_thread();

  bool init();
  bool writeData(QByteArray data, tcp_connection * receiver);

  Q_SLOT void disconnected();
  Q_SLOT void readFromClient();
  Q_SLOT void sendMessage(QString, tcp_connection * request);
  Q_SLOT void acceptConnection();
  Q_SLOT void stop() {m_continue = false;}
  Q_SLOT void send_pair_info(tcp_connection * request);
  Q_SLOT void disconnect_client(tcp_connection *, QString *);
  Q_SIGNAL void readIt(QTcpSocket *);
  Q_SIGNAL void receivedMessage();

  Q_SIGNAL void got_create_account(QString *, QTcpSocket *);
  Q_SIGNAL void got_login_request(QString *, QTcpSocket *);
  Q_SIGNAL void got_create_group(QString *, QTcpSocket *);
  Q_SIGNAL void got_join_group(QString *, QTcpSocket *);
  Q_SIGNAL void got_leave_group(QString *, QTcpSocket *);
  Q_SIGNAL void got_update_user(QString *, QTcpSocket *);
  Q_SIGNAL void got_request_groups(QString *, QTcpSocket *);
  Q_SIGNAL void got_request_account(QString *, QTcpSocket *);
  Q_SIGNAL void got_delete_group(QString *, QTcpSocket *);
  Q_SIGNAL void got_list_group_users(QString *, QTcpSocket *);
  Q_SIGNAL void got_create_user_event(QString *, QTcpSocket *);
  Q_SIGNAL void got_create_group_event(QString *, QTcpSocket *);
  Q_SIGNAL void got_reset_password(QString *, QTcpSocket *);
  Q_SIGNAL void got_request_events(QString *, QTcpSocket *);
  Q_SIGNAL void got_request_month_events(QString *, QTcpSocket *);
  Q_SIGNAL void got_request_group_events(QString *, QTcpSocket *);
  Q_SIGNAL void got_request_group_month_events(QString *, QTcpSocket *);
  Q_SIGNAL void got_create_friendship(QString *, QTcpSocket *);
  Q_SIGNAL void got_accept_friend(QString *, QTcpSocket *);
  Q_SIGNAL void got_request_friends(QString *, QTcpSocket *);
  Q_SIGNAL void got_delete_friend(QString *, QTcpSocket *);
  Q_SIGNAL void got_friend_requests(QString *, QTcpSocket *);
  Q_SIGNAL void got_present(QString *, QTcpSocket *);
  Q_SIGNAL void got_absent(QString *, QTcpSocket *);
  Q_SIGNAL void got_suggest_user_time(QString *, QTcpSocket *);
  Q_SIGNAL void got_suggest_group_times(QString *, QTcpSocket *);

  Q_SIGNAL void worker_connected(worker_connection * _worker);
  Q_SIGNAL void client_connected(client_connection * _client);

  Q_SIGNAL void dropped_connection(tcp_connection *);
  Q_SIGNAL void dropped_client();

  Q_SLOT void echoReceived(QString);
  Q_SLOT void timeout_disconnect();

  int queueDepth()
  {
    QMutex * pMutex = new QMutex();
    pMutex->lock();
    int queueSize = m_pTcpMessages->size();
    int size = 0;
    if (!queueSize) {
      pMutex->unlock();
      delete pMutex;
      return queueSize;
    } else {
      for (int x = 0; x < queueSize; ++x) {
        ++size;
      }
    }
    pMutex->unlock();
    delete pMutex;
    return size;
  }

  void set_master(master_node * _p_master_node) {m_p_master_node = _p_master_node;}
  void set_worker(worker_node * _p_worker_node) {m_p_worker_node = _p_worker_node;}

  const master_node * get_master() {return m_p_master_node;}
  const worker_node * get_worker() {return m_p_worker_node;}

  const QTcpServer * getServer() {return m_pServer;}

private:
  QTcpServer * m_pServer;

  volatile bool m_continue = true;
  volatile bool m_master_mode = false;

  QString m_hostname;
  quint16 m_port;
  quint16 m_blockSize;

  master_node * m_p_master_node;
  worker_node * m_p_worker_node;

  QTcpSocket * currentSocket = NULL;
  QQueue<tcp_connection> * m_pTcpMessages;
  QList<tcp_connection *> m_tcp_connections;
  QTimer * m_p_timer;
};
#endif
