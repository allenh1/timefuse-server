#include "master.hpp"

/** 
 * Construct the master node for the timefuse-server.
 * 
 * @brief constructor
 *
 * @param _hostname Hostname for the master.
 * @param _port Port on which to construct the master.
 * @param _p_parent Parent QObject
 */
master_node::master_node(const QString & _hostname, const quint16 & _port, QObject * _p_parent)
   : m_hostname(_hostname),
	 m_port(_port),
	 QObject(pParent)
{ /* @todo Create the master */ }

/** 
 * @brief Initialize the master node.
 *
 * This function does a number of things
 * that need to be done before using this
 * node. Namely, this function constructs
 * this thread, constructs the tcp_thread,
 * connects the handlers to tcp_thread's
 * signals, and, lastly, moves us onto
 * the thread we constructed.
 *
 * @return True upon successful initialization.
 */
bool master_node::init()
{
   /**
	* @todo construct & move onto thread, as well as
	* set up tcp thread and connect handlers.
	*/
}

/** 
 * @brief Handler for client connection.
 *
 * This function enqueue's the incoming
 * client connection in a thread-safe
 * manner.
 * 
 * @param _client Newly connected client.
 */
void master_node::handle_client_connect(client_connection * _client)
{
   /* lock the mutex guarding our semaphore */
   m_p_client_mutex->lock();
   /* release resources in the semaphore */
   m_p_client_sema->release();
   /* enqueue the client */
   m_client_connections.enqueue(_client);
   /* unlock the mutex */
   m_p_client_mutex->unlock();
}

/** 
 * @brief Handler for worker connection.
 *
 * This function enqueue's the incoming
 * worker connection in a thread-safe
 * manner.
 * 
 * @param _worker Newly connected worker.
 */
void master_node::handle_worker_connect(worker_connection * _worker)
{
   /* lock the mutex guarding our semaphore */
   m_p_worker_mutex->lock();
   /* release resources in the semaphore */
   m_p_worker_sema->release();
   /* enqueue the worker */
   m_worker_connections.enqueue(_worker);
   /* unlock the mutex */
   m_p_worker_mutex->unlock();
}




