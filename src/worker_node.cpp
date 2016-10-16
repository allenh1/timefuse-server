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
   m_p_tcp_thread = new tcp_thread(m_hostname, m_port);

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

   for (; m_continue; m_p_thread->msleep(sleep_time)) {
	  /**
	   * @todo Implement the different things to do
	   */
   }
}

/**
 *
 */
QSqlDatabase setup_db() {
   const char *user, *pwd, *dbb, *host;
   if ((user = getenv("DBUSR")) == NULL) {
      perror("getenv");
      throw std::invalid_argument( "getenv on user failed" );
   } else if ((pwd = getenv("DBPASS")) == NULL) {
      perror("getenv");
      throw std::invalid_argument( "getenv on pwd failed" );
   } else if ((pwd = getenv("DBNAME")) == NULL) {
      perror("getenv");
      throw std::invalid_argument( "getenv on db name failed" );
   } else if((pwd = getenv("DBHOST")) == NULL) {
      perror("getenv");
      throw std::invalid_argument( "getenv on db host failed" );
   }

   QSqlDatabase db = QSqlDatabase::addDatabase("QMYSQL");

   db.setHostName(host); db.setDatabaseName(dbb);
   db.setUserName(user); db.setPassword(pwd);

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
      std::cerr<<"Error! Fialed to open database connection!"<<std::endl;
      return false;
   }

   QSqlQuery * query = new QSqlQuery(db);

   query->prepare("INSERT INTO users (user_id, schedule_id, user_name passwd, email)"
		  "VALUES (:user_id, :schedule_id, :user_name, :passwd, :email)");
   query->bindValue(":user_id", u.get_user_id());
   query->bindValue(":schedule_id", u.get_schedule_id());
   query->bindValue(":user_name", u.get_username());
   query->bindValue(":paswd", u.get_password());
   query->bindValue(":email", u.get_email());

   if((query->exec()) == NULL) {
      perror("exec");
      throw std::invalid_argument( "something failed in the insert query" );
      return false;
   }
   return true;
}
