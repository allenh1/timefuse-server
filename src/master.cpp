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
{ /* master is created in init */ }

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
   /* construct our thread */
   m_p_thread = new QThread();
   /* construct the tcp_thread */
   m_p_tcp_thread = new tcp_thread(m_hostname, m_port);

   /* establish connection handlers */
   connect(m_p_tcp_thread, &tcp_thread::client_connected,
		   this, &master_node::handle_client_connect);
   connect(m_p_tcp_thread, &tcp_thread::worker_connected,
		   this, &master_node::handle_worker_connect);

   /* move onto the constructed thread */
   this->move_to_thread(m_p_thread);
   /* set the run loop to be ours */
   connect(m_p_thread, &QThread::started,
		   this, &master_node::run);
   /* finally, start the thread. */
   m_p_thread.start();
   return true;
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

/** 
 * @brief run function for this thread
 *
 * This is the main run loop for the master
 * thread. The master loops through resources,
 * while necessary and pairs a client to
 * a worker when both are available.
 */
void master_node::run()
{
   const quint16 sleep_time = 100;
   
   for (; m_continue; m_p_thread->m_sleep(sleep_time)) {
	  /* lock client mutex */
	  m_p_client_mutex->lock();
	  
	  /* check that our queue is non-empty */
	  if (!p_client_queue.size()) {
		 /* unlock mutex and continue */
		 p_client_mutex->unlock();
		 continue;
	  }

	  /* now try to acquire a client */
	  m_p_client_sema->acquire();
	  /* dequeue the client */
	  client_connection * c = m_client_connecitons.dequeue();
	  /* unlock client mutex */
	  m_p_client_mutex->unlock();

	  /* lock worker mutex */
	  m_p_worker_mutex->lock();

	  /* check that the worker queue is non-empty */
	  if (!p_worker_queue.size()) {
		 /* add the client back and continue */
		 handle_client_connect(c);
		 /* unlock worker mutex */
		 m_p_worker_mutex->unlock();
		 continue;
	  }

	  /* try to acquire a worker */
	  m_p_worker_sema->acquire();
	  worker_connection * w = m_worker_connections.dequeue();
	  /* unlock the worker mutex */
	  m_p_worker_mutex->unlock();

	  /* send client to worker */
	  w->add_client(c);

	  /* send worker to client */
	  c->add_worker(w);

	  /* disconnect worker and client */
	  w->disconnect(); c->disconnect();
   }
}
