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

#include "worker_connection.hpp"

worker_connection::worker_connection(
  QString & _hostname,
  QTcpSocket * _p_socket,
  QObject * _p_parent)
: tcp_connection(_hostname, _p_socket, _p_parent)
{ /* constructor implemented in base class */}

worker_connection::~worker_connection()
{ /* @todo to delete, or not to delete? */}

void worker_connection::add_client(client_connection * c)
{
  /**
       * @todo Implement the add client connection
       */
  QTcpSocket * peer = (QTcpSocket *) c->get_socket();
  QHostAddress client_address(peer->peerAddress().toIPv4Address());
  quint16 client_port = peer->peerPort();

  std::cout << "Added client at " << client_address.toString().toStdString() << ":" <<
    client_port << std::endl;
}
