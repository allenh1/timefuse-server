#ifndef __WORKER_CONNECTION_HPP__
#define __WORKER_CONNECTION_HPP__
#include "tcp_connection.hpp"
#include "client_connection.hpp"

class client_connection;

class worker_connection : public tcp_connection
{
  Q_OBJECT

public:
  explicit worker_connection(
    QString & _hostname,
    QTcpSocket * _p_socket,
    QObject * pParent = NULL
  );
  virtual ~worker_connection();

  void add_client(client_connection * c);
};
#endif
