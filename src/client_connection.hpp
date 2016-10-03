#ifndef __CLIENT_CONNECTION_HPP__
#define __CLIENT_CONNECTION_HPP__
#include "tcp_conneciton.hpp"

class client_connection : public tcp_connection
{
   Q_OBJECT
public:
   explicit client_connection(QString & _hostname,
							  QTcpSocket * _pSocket,
							  QObject * pParent = NULL
	  );
   virtual ~client_connection();

   void add_worker(const worker_connection *& w);
};
#endif
