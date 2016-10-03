#ifndef __WORKER_CONNECTION_HPP__
#define __WORKER_CONNECTION_HPP__
#include "tcp_connection.hpp"

class worker_connection : public tcp_connection
{
   Q_OBJECT
public:
   explicit worker_connection(QString & _hostname,
							  QTcpSocket * _p_socket,
							  QObject * pParent = NULL
	  );
   virtual ~worker_connection();

   void add_client(const client_connection *& c);
};
#endif
