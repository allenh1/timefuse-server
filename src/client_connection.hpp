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

#ifndef __CLIENT_CONNECTION_HPP__
#define __CLIENT_CONNECTION_HPP__
#include "worker_connection.hpp"

class worker_connection;

class client_connection : public tcp_connection
{
  Q_OBJECT

public:
  explicit client_connection(
    QString & _hostname,
    QTcpSocket * _p_socket,
    QObject * pParent = NULL
  );
  virtual ~client_connection();

  void add_worker(worker_connection * w);
  const QString & get_paired_hostname() {return m_paired_host;}

private:
  QString m_paired_host;
};
#endif
