#include "worker_connection.hpp"

worker_connection::worker_connection(QString & _hostname,
									 QTcpSocket * _p_socket,
									 QObject * _p_parent)
   : tcp_connection(_hostname, _p_socket, _p_parent)
{ /* constructor implemented in base class */ }

worker_connection::~worker_connection()
{ /* @todo to delete, or not to delete? */ }

void worker_connection::add_client(const client_connection * c)
{
   /**
	* @todo Implement the add client connection
	*/
}
