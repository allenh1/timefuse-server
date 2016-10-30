#include "worker_node.hpp"

worker_node::worker_node(const QString & _host, const quint16 & _port, QObject * _p_parent)
	: QObject(_p_parent),
	  m_host(_host),
	  m_port(_port),
	  m_p_mutex(new QMutex())
{
	/**
	 * @todo verify nothing else needs to be done here.
	 */
	m_db = setup_db();
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
	connect(this, &worker_node::disconnect_client, m_p_tcp_thread, &tcp_thread::disconnect_client);
	connect(m_p_tcp_thread, &tcp_thread::got_create_account,
			this, &worker_node::request_create_account,
			Qt::DirectConnection);
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
	bool served;
	
	for (; m_continue; m_p_thread->msleep(sleep_time)) {
		if (state == connection_state::CONNECT_TO_MASTER) goto connect_to_master;
		else if (state == connection_state::WAIT_FOR_JOB) goto wait_for_job;
		else if (state == connection_state::WAIT_FOR_CLIENT_CONNECT) goto wait_for_client;
	connect_to_master:
		served_client = false;
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
		port_string.setNum(m_port);
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
		} m_p_mutex->lock();
		served_client = false;
		m_p_mutex->unlock();
	wait_for_client:
		for (;;m_p_thread->msleep(sleep_time)) {
			m_p_mutex->lock();
			served = served_client;
			m_p_mutex->unlock();
			if (served) goto end;
		}
	end:
		state = connection_state::CONNECT_TO_MASTER;
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
 * @brief insert a user to the database.
 *
 * @param u User to insert
 * @return True if the insertion succeeded.
 */
bool worker_node::insert_user(user & u) {
	if(!m_db.open()) {
		std::cerr<<"Error! Failed to open database connection!"<<std::endl;
		return false;
	} else if (!u.get_username().size()) return false;

	QSqlQuery * query = new QSqlQuery(m_db); 
	QString user_query_string = "INSERT INTO users(user_name, schedule_id, passwd, email)";
	QString schedule_item_string = "INSERT INTO schedules(owner) VALUES(";
	schedule_item_string += "'" + u.get_username() + "');";

	query->prepare(schedule_item_string);
	if(!query->exec()) {
		std::cerr<<"Query Failed to execute!"<<std::endl;
		std::cerr<<"query: \""<<query->lastQuery().toStdString()<<"\""<<std::endl;
		delete query;
		throw std::invalid_argument("something failed in the insert query");
		return false;
	} delete query;

	if (!select_schedule_id(u)) throw std::invalid_argument("Something bad happened!");
   
	user_query_string += " VALUES('" + u.get_username() + "', '" + u.get_schedule_id() 
		+  "', '" + u.get_password() + "', '" + u.get_email() + "');";

	query = new QSqlQuery(m_db);
	query->prepare(user_query_string);
	if(!query->exec()) {
		std::cerr<<"Query Failed to execute!"<<std::endl;
		std::cerr<<"query: \""<<query->lastQuery().toStdString()<<"\""<<std::endl;
		std::string str = "Something failed in insert query:\n"
			+ query->lastQuery().toStdString();
		delete query;
		throw std::invalid_argument(str);
		return false;
	} delete query;
	return true;
}

bool worker_node::username_exists(const QString & _user)
{
	if (!m_db.open()) {
		std::cerr<<"Error! Failed to open database connection!"<<std::endl;
		/**
		 * @todo probably shouldn't return false here
		 */
		return true;
	}

	QSqlQuery * query = new QSqlQuery(m_db);
	QString text = "SELECT * FROM users WHERE user_name = '"
		+ _user + "';";

	if (!query->exec(text)) {
		std::cerr<<"Query failed to execute!"<<std::endl;
		std::cerr<<"query: \""<<query->lastQuery().toStdString()<<"\""<<std::endl;
		text = "Something failed in select query:\n" + text;
		std::string str = text.toStdString(); delete query;
		throw std::invalid_argument(str);
		/**
		 * @todo again, probably should not return false.
		 */
		return true;
	} else if (query->size()) return true;
	return false;
}

bool worker_node::select_schedule_id(user & u)
{
	if(!m_db.open()) {
		std::cerr<<"Error! Failed to open database connection!"<<std::endl;
		return false;
	}

	QSqlQuery * query = new QSqlQuery(m_db);
   
	QString schedule_select = "SELECT schedule_id FROM schedules WHERE ";
	schedule_select += "owner = '" + u.get_username() + "';";

	if (!query->exec(schedule_select)) {
		std::cerr<<"Query failed to execute!"<<std::endl;
		std::cerr<<"query: \""<<query->lastQuery().toStdString()<<"\""<<std::endl;
		schedule_select = "Something failed in select query:\n" + schedule_select;
		std::string str = schedule_select.toStdString(); delete query;
		throw std::invalid_argument(str);
		return false;
	} else if (!query->size()) return false;

	/* now extract the schedule id and set in our referenced object */
	register int sched_id_col = query->record().indexOf("schedule_id");
	query->next();
	if (sched_id_col != -1) u.set_schedule_id(query->value(sched_id_col).toString());
	else throw std::invalid_argument("No schedule_id column returned");
	return true;
}

bool worker_node::select_user(user & u) {
    if(!m_db.open()) {
		std::cerr<<"Error! Failed to open database connection!"<<std::endl;
		return false;
	}

    QSqlQuery * query = new QSqlQuery(m_db);
   
	QString user_stuff = "SELECT user_id, schedule_id, email, cellphone FROM users WHERE ";
	user_stuff += "user_name = '" + u.get_username() + "' AND passwd = '" + u.get_password() + "';";

	if(!query->exec(user_stuff)) {
		std::cerr<<"Query Failed to execute!"<<std::endl;
		std::cerr<<"query: \""<<query->lastQuery().toStdString()<<"\""<<std::endl;
		delete query;
		user_stuff = "Something failed in select query:\n" + user_stuff;
		std::string str = user_stuff.toStdString();
		throw std::invalid_argument(str);
		return false;
	} else if (!query->size()) return false;
	
	int id_col = query->record().indexOf("user_id");
	int sched_id_col = query->record().indexOf("schedule_id");
	int email_col = query->record().indexOf("email");
	int cell_col = query->record().indexOf("cellphone");
	QVariant user_id, schedule_id, email, cellphone;

	for (; query->next(); ) {
		if (id_col != -1) user_id = query->value(id_col);
		else throw std::invalid_argument("No user_id column returned");
   
		if (sched_id_col != -1) schedule_id = query->value(sched_id_col);
		else throw std::invalid_argument("No schedule_id column returned");

		if (email_col != -1) email = query->value(email_col);
		else throw std::invalid_argument("No email column returned");

		if (cell_col != -1) cellphone = query->value(cell_col);

		QString db_email = email.toString();
		QString db_user_id = user_id.toString();
		QString db_schedule_id = schedule_id.toString();
		QString db_cell = cellphone.toString();
		u.set_email(db_email);
		u.set_user_id(db_user_id);
		if (db_cell.size()) u.set_cell(db_cell);
		u.set_schedule_id(db_schedule_id);
	} delete query;
	return true;
}

/**
 * @brief Try to create an account.
 *
 * @param _user Encrypted username.
 * @param _password Encrypted password.
 * @param _email Unencrypted email
 * @return True upon successful creation.
 */
bool worker_node::try_create(const QString & _user, const QString & _password, const QString & _email)
{
	user u; u.set_email(_email);
	u.set_username(_user); u.set_password(_password);

	return !username_exists(_user) && insert_user(u);
}

/**
 * @brief Try to login.
 *
 * @param _user Encrypted username.
 * @param _password Encrypted password.
 * @return True upon authentication.
 */
bool worker_node::try_login(const QString & _user, const QString & _password)
{
	user u; /* temporary user to fill. set username and password. */
	u.set_username(_user); u.set_password(_password);

	return select_user(u);
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
	if(!m_db.open()) {
		std::cerr<<"Error! Failed to open database connection!"<<std::endl;
		return false;
	}
	/* now we remove the inserted */
	QString delete_user = "DELETE FROM users WHERE user_name = 'billy'";
	QString delete_schedule_item = "DELETE FROM schedules WHERE owner = 'billy'";
	QSqlQuery * query = new QSqlQuery(m_db);
	query->prepare(delete_user);

	if (!query->exec()) {
		std::cerr<<"Query Failed to execute!"<<std::endl;
		std::cerr<<"query: \""<<query->lastQuery().toStdString()<<"\""<<std::endl;
		delete query;
		throw std::invalid_argument("something failed in deleting the schedule");
		return false;
	} delete query;

	query = new QSqlQuery(m_db);
	query->prepare(delete_schedule_item);

	if (!query->exec()) {
		std::cerr<<"Query Failed to execute!"<<std::endl;
		std::cerr<<"query: \""<<query->lastQuery().toStdString()<<"\""<<std::endl;
		delete query;
		throw std::invalid_argument("something failed in deletion of a user.");
		return false;
	} delete query;
	return true;
}

void worker_node::request_create_account(QString * _p_text,
										 QTcpSocket * _p_socket)
{
	std::cout<<"create account request: \""<<_p_text->toStdString()<<"\""<<std::endl;
	/* create a tcp_connection object */
	QString client_host = _p_socket->peerName();
	tcp_connection * p = new tcp_connection(client_host, _p_socket);
	
	/* split along ':' characters */
	QStringList separated = _p_text->split(":");
	
	if (separated.size() < 3) {
		/* if there are not enough params, disconnect. */
		QString * msg = new QString("ERROR: INVALID REQUEST\r\n");
		Q_EMIT(disconnect_client(p, msg));
	}

	QString _user = separated[0];
	QString _pass = separated[1];
	QString _mail = separated[2];
	QString _cell = (separated.size() > 3) ? separated[3] : "";

	if (username_exists(_user)) {
		/* the user exists. Invalid request. */
		QString * msg = new QString("ERROR: EXISTING USER\r\n");
		Q_EMIT(disconnect_client(p, msg));
	}
	QString * msg;
	/* try to insert the user */
	try {
		
		if (!try_create(_user, _pass, _mail)) {
			msg = new QString("ERROR: DB INSERT FAILED\r\n");
		} else msg = new QString("OK\r\n");
	} catch ( ... ) {
		msg = new QString("ERROR: DB INSERT FAILED\r\n");
	} Q_EMIT(disconnect_client(p, msg));
	m_p_mutex->lock();
	served_client = true;
	m_p_mutex->unlock();
}
