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

#include "client_connection.hpp"


/**
 * @brief Constructs a client_connection.
 *
 * @param _hostname Hostname of the client.
 * @param _p_socket TcpSocket on which we find the client.
 * @param _p_parent Parent QObject.
 */
client_connection::client_connection(
  QString & _hostname,
  QTcpSocket * _p_socket,
  QObject * pParent)
: tcp_connection(_hostname, _p_socket, pParent)
{ /* forward to the base class */}

client_connection::~client_connection()
{ /* Not sure I need to delete anything? */}

/**
 * @brief Notify client of its worker assignment.
 *
 * This function will tell the client who its
 * worker is. It should send the necessary info
 * (as of now, this means port and hostname)
 * to the worker.
 *
 * @param w Worker to which we are being assigned.
 */
void client_connection::add_worker(worker_connection * w)
{
  std::cout << "Added worker location: " <<
  (m_paired_host = w->get_hostname()).toStdString() <<
    std::endl;
}
