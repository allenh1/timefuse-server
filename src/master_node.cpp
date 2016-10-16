#include "master_node.hpp"

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
   : QObject(_p_parent),
	 m_hostname(_hostname),
	 m_port(_port)
{
   /* create mutexes */
   m_p_client_mutex = new QMutex();
   m_p_worker_mutex = new QMutex();

   /* create semaphores */
   m_p_client_sema = new QSemaphore();
   m_p_worker_sema = new QSemaphore();
}

master_node::~master_node()
{
   /**
	* @todo free resources and halt threads
	*/
}

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
   std::cout<<"Initializing master thread..."<<std::endl;
   /* construct our thread */
   m_p_thread = new QThread();
   /* construct the tcp_thread */
   m_p_tcp_thread = new tcp_thread(m_hostname, m_port);
   m_p_tcp_thread->init();

   /* give tcp thread a pointer to us */
   m_p_tcp_thread->set_master(this);

   std::cout<<"Moving onto constructed thread..."<<std::endl;
   /* move onto the constructed thread */
   this->moveToThread(m_p_thread);
   /* set the run loop to be ours */
   connect(m_p_thread, &QThread::started,
		   this, &master_node::run);
   /* finally, start the thread. */
   m_p_thread->start();
   return m_p_thread->isRunning();
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
   std::cerr<<"caught client connect"<<std::endl;
   /* lock the mutex guarding our semaphore */
   m_p_client_mutex->lock();
   /* release resources in the semaphore */
   m_p_client_sema->release();
   /* enqueue the client */
   m_client_connections.enqueue(_client);
   /* unlock the mutex */
   m_p_client_mutex->unlock();
   std::cerr<<"added new client"<<std::endl;
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
   std::cerr<<"caught worker connect"<<std::endl;
   /* lock the mutex guarding our semaphore */
   m_p_worker_mutex->lock();
   /* release resources in the semaphore */
   m_p_worker_sema->release();
   /* enqueue the worker */
   m_worker_connections.enqueue(_worker);
   /* unlock the mutex */
   m_p_worker_mutex->unlock();
   std::cerr<<"added new worker"<<std::endl;
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

   std::cout<<"Master Thread started"<<std::endl;
   
   for (; m_continue; m_p_thread->msleep(sleep_time)) {
	  /* lock client mutex */
	  m_p_client_mutex->lock();
	  
	  /* check that our queue is non-empty */
	  if (!m_client_connections.size()) {
		 /* unlock mutex and continue */
		 m_p_client_mutex->unlock();
		 continue;
	  }

	  /* now try to acquire a client */
	  m_p_client_sema->acquire();
	  /* dequeue the client */
	  client_connection * c = m_client_connections.dequeue();
	  /* unlock client mutex */
	  m_p_client_mutex->unlock();

	  /* lock worker mutex */
	  m_p_worker_mutex->lock();

	  /* check that the worker queue is non-empty */
	  if (!m_worker_connections.size()) {
		 /* add the client back and continue */
		 handle_client_connect(c);
		 /* unlock worker mutex */
		 m_p_worker_mutex->unlock();
		 /* lock the worker mutex */
		 m_p_worker_mutex->lock();
		 /* release resources in the semaphore */
		 m_p_client_sema->release();
		 /* enqueue the client */
		 m_client_connections.enqueue(c);
		 /* unlock the mutex */
		 m_p_client_mutex->unlock();
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
   }
}
