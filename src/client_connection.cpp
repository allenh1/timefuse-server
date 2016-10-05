#include "client_connection.hpp"


/** 
 * @brief Constructs a client_connection.
 * 
 * @param _hostname Hostname of the client.
 * @param _p_socket TcpSocket on which we find the client.
 * @param _p_parent Parent QObject.
 */
client_connection::client_connection(QString & _hostname,
									 QTcpSocket * _p_socket,
									 QObject * pParent)
   : tcp_connection(_hostname, _p_socket,  pParent)
{ /* forward to the base class */ }

client_connection::~client_connection()
{ /* Not sure I need to delete anything? */ }

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
void client_connection::add_worker(const worker_connection * w)
{
   /**
	* @todo implement this function
	*/
}
