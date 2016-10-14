#include "worker_node.hpp"

worker_node::worker_node(const QString & _hostname, const quint16 & _port, QObject * _p_parent)
   : QObject(_p_parent),
	 m_hostname(_hostname),
	 m_port(_port)
{
   /**
    * @todo verify nothing else needs to be done here.
    */
}

worker_node::~worker_node()
{
   /**
    * @todo nothing to be done as of right now
	*/
}

bool worker_node::init()
{
   std::cout<<"Initializing worker thread..."<<std::endl;

   /* construct the thread */
   m_p_thread = new QThread();
   /* construct the tcp thread */
   m_p_tcp_thread = new tcp_thread(m_hostname, m_port, false);

   /* if the tcp thread fails to start, throw an exception. */
   if (!m_p_tcp_thread->init()) throw thread_init_exception("tcp_thread failed to initialize.");

   /* give tcp thread a pointer to this thread */
   m_p_tcp_thread->set_worker(this);

   std::cout<<"Moving onto constructed thread..."<<std::endl;
   /* move onto the constructed thread */
   this->moveToThread(m_p_thread);

   /* connect signals */
   connect(m_p_thread, &QThread::started, this, &worker_node::run);
   connect(this, &worker_node::finished_client_job, this, &worker_node::stop);
   connect(this, &worker_node::established_client_connection, this, &worker_node::start_thread);
   
   return true;
}

void worker_node::run()
{
   std::cout<<"Worker Thread started"<<std::endl;

   /* initialize the state machine with a connect to master */
   state = connection_state::CONNECT_TO_MASTER;
   
   for (; m_continue; m_p_thread->msleep(sleep_time)) {
	  if (state == connection_state::CONNECT_TO_MASTER) goto connect_to_master;
	  else if (state == connection_state::WAIT_FOR_JOB) goto wait_for_job;
	  else if (state == connection_state::WAIT_FOR_CLIENT_CONNECT) goto connect_client;
	  else if (state == connection_state::PROCESS_JOB) goto process_job;
	  else if (state == connection_state::DISCONNECT_CLIENT) goto disconnect_client;
	  else goto disconnect_client;
	  
   connect_to_master:
	  /* @todo */
   wait_for_job:
	  /* @todo */
   connect_client:
	  /* @todo */
   process_job:
	  /* @todo */
   disconnect_client:
	  /* @todo */

   end:
	  continue; /* go around again */
   }
}
