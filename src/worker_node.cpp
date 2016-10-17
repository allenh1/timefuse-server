#include "worker_node.hpp"

worker_node::worker_node(const QString & _host, const quint16 & _port, QObject * _p_parent)
   : QObject(_p_parent),
	 m_host(_host),
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
   m_p_tcp_thread = new tcp_thread(m_host, m_port, false);

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

   m_p_thread->start();
   return m_p_thread->isRunning();
}

void worker_node::run()
{
   std::cout<<"Worker Thread started"<<std::endl;

   /* initialize the state machine with a connect to master */
   state = connection_state::CONNECT_TO_MASTER;

   QTcpSocket * pSocket = NULL;
   QString read, our_tcp_server, port_string;
   
   for (; m_continue; m_p_thread->msleep(sleep_time)) {
	  if (state == connection_state::CONNECT_TO_MASTER) goto connect_to_master;
	  else if (state == connection_state::WAIT_FOR_JOB) goto wait_for_job;
	  else if (state == connection_state::WAIT_FOR_CLIENT_CONNECT) goto connect_client;
	  else if (state == connection_state::PROCESS_JOB) goto process_job;
	  else if (state == connection_state::DISCONNECT_CLIENT) goto disconnect_client;
	  else goto disconnect_client;
	  
   connect_to_master:
	  std::cout<<"state: CONNECT_TO_MASTER"<<std::endl;
	  if (pSocket != NULL) delete pSocket;
	  pSocket = new QTcpSocket();
	  
	  pSocket->connectToHost(m_master_host,
							 m_master_port,
							 QIODevice::ReadWrite);
	  pSocket->waitForConnected(tcp_comm::TIMEOUT);

	  if (pSocket->state() == QAbstractSocket::UnconnectedState) {
		 pSocket->abort();
		 delete pSocket;
		 pSocket = NULL; /* asign to null for next go around */
		 goto end;
	  }

	  /* write the greeting message to the master_node */
	  pSocket->write("REQUEST_CLIENT\r\n");
	  /* write our location to the server */
	  our_tcp_server = m_host + ":";
	  port_string; port_string.setNum(m_port);
	  our_tcp_server += port_string + "\r\n" + '\0';
	  pSocket->write(our_tcp_server.toStdString().c_str()); /* write the next line */
	  
	  /* wait until the bytes have been written, unlimited time. */
	  pSocket->waitForBytesWritten(-1);

	  /* if we are here, we can proceed. */
	  state = connection_state::WAIT_FOR_JOB;
   wait_for_job:
	  std::cout<<"state: WAIT_FOR_JOB"<<std::endl;
	  /* wait for ready read */
	  pSocket->waitForReadyRead();
	  for (; pSocket->canReadLine(); read += pSocket->readLine());

	  /* check that we have things. Otherwise, loop again. */
	  if (read.size() == 0) goto end;
	  read.replace("\r\n", "");
	  std::cout<<"I read \""<<read.toStdString()<<"\""<<std::endl;
	  if (read == "OK") {
		 state = connection_state::WAIT_FOR_CLIENT_CONNECT;
		 pSocket->disconnectFromHost();
		 delete pSocket;
		 pSocket = NULL;
		 goto connect_client;
	  }
	  /* now disconnect from the master */
   disconnect_master:
	  /* @todo */
   connect_client:
	  goto end;
   process_job:
	  /* @todo */
   disconnect_client:
	  /* @todo */

   end:
	  continue; /* go around again */
   }
}

/**
 * Unter doesn't know how to spell worker. (Unter=Hunter)
 */
QSqlDatabase worker_node::setup_db() {
   const char *user, *pwd, *dbb, *host, *port_string;
   if ((user = getenv("DBUSR")) == NULL) {
      perror("getenv");
      throw std::invalid_argument( "getenv on user failed" );
   } else if ((pwd = getenv("DBPASS")) == NULL) {
      perror("getenv");
      throw std::invalid_argument( "getenv on pwd failed" );
   } else if ((dbb = getenv("DBNAME")) == NULL) {
      perror("getenv");
      throw std::invalid_argument( "getenv on db name failed" );
   } else if ((host = getenv("DBHOST")) == NULL) {
      perror("getenv");
      throw std::invalid_argument( "getenv on db host failed" );
   } else if ((port_string = getenv("DBPORT")) == NULL) {
	  perror("getenv");
	  throw std::invalid_argument("getenv on db host failed");
   }

   QSqlDatabase db = QSqlDatabase::addDatabase("QMYSQL");
   quint64 port = std::stoi(std::string(port_string));
							
   db.setHostName(host); db.setDatabaseName(dbb);
   db.setUserName(user); db.setPassword(pwd);
   db.setPort(port);
   return db;
}


/**
 * create a new user in the database
 *
 * @param db
 * @param user
 */
bool worker_node::insert_query(user & u) {
   QSqlDatabase db = setup_db();

   if(!db.open()) {
      std::cerr<<"Error! Failed to open database connection!"<<std::endl;
      return false;
   }

   QSqlQuery * query = new QSqlQuery(db); 
   QString user_query_string = "INSERT INTO users(user_id, schedule_id, user_name, passwd, email)";
   QString schedule_item_string ="INSERT INTO schedules(schedule_id) VALUES ('-1');";
   user_query_string += " VALUES('" + u.get_user_id() + "', '" + u.get_schedule_id() + "', '" + u.get_username()
 	                 +  "', '" + u.get_password() + "', '" + u.get_email() + "');";

   query->prepare(schedule_item_string);
   if((query->exec()) == NULL) {
	  std::cerr<<"Query Failed to execute!"<<std::endl;
	  std::cerr<<"query: \""<<query->lastQuery().toStdString()<<"\""<<std::endl;
	  delete query;
	  throw std::invalid_argument("something failed in the insert query");
	  return false;
   } delete query;

   query = new QSqlQuery(db);
   query->prepare(user_query_string);
   if((query->exec()) == NULL) {
	  std::cerr<<"Query Failed to execute!"<<std::endl;
	  std::cerr<<"query: \""<<query->lastQuery().toStdString()<<"\""<<std::endl;
	  delete query;
	  throw std::invalid_argument("something failed in the insert query");
	  return false;
   } delete query;
   return true;
}

/**
 * @brief cleanup after testcase
 *
 * DO NOT CALL THIS FUNCTION. This is
 * strictly for cleaning up after a test case.
 *
 * @return true, we hope.
 */
bool worker_node::cleanup_db_insert()
{
    QSqlDatabase db = setup_db();

   if(!db.open()) {
      std::cerr<<"Error! Failed to open database connection!"<<std::endl;
      return false;
   }
   /* now we remove the inserted */
   QString delete_user = "DELETE FROM users WHERE user_id = '-1'";
   QString delete_schedule_item = "DELETE FROM schedules WHERE schedule_id = '-1'";
   QSqlQuery * query = new QSqlQuery(db);
   query->prepare(delete_user);

   if((query->exec()) == NULL) {
	  std::cerr<<"Query Failed to execute!"<<std::endl;
	  std::cerr<<"query: \""<<query->lastQuery().toStdString()<<"\""<<std::endl;
	  delete query;
	  throw std::invalid_argument("something failed in deleting the schedule");
	  return false;
   } delete query;

   query = new QSqlQuery(db);
   query->prepare(delete_schedule_item);

   if((query->exec()) == NULL) {
	  std::cerr<<"Query Failed to execute!"<<std::endl;
	  std::cerr<<"query: \""<<query->lastQuery().toStdString()<<"\""<<std::endl;
	  delete query;
	  throw std::invalid_argument("something failed in deletion of a user.");
	  return false;
   } delete query;
   return true;
}
