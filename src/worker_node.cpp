#include "worker_node.hpp"

worker_node::worker_node(const QString & _host, const quint16 & _port, QObject * _p_parent)
	: QObject(_p_parent),
	  m_host(_host),
	  m_port(_port),
	  m_db(setup_db()),
	  m_p_mutex(new QMutex())	  
{ /* constructor */ }

worker_node::~worker_node()
{
    delete m_p_mutex;
	delete m_p_tcp_thread;
	delete m_p_thread;
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

	/* connect message handlers */
	connect(m_p_tcp_thread, &tcp_thread::got_create_account,
			this, &worker_node::request_create_account,
			Qt::DirectConnection);
	connect(m_p_tcp_thread, &tcp_thread::got_login_request,
			this, &worker_node::request_login,
			Qt::DirectConnection);
	connect(m_p_tcp_thread, &tcp_thread::got_create_group,
			this, &worker_node::request_create_group,
			Qt::DirectConnection);
	connect(m_p_tcp_thread, &tcp_thread::got_join_group,
			this, &worker_node::request_add_to_group,
			Qt::DirectConnection);
	connect(m_p_tcp_thread, &tcp_thread::got_leave_group,
			this, &worker_node::request_leave_group,
			Qt::DirectConnection);
	connect(m_p_tcp_thread, &tcp_thread::got_update_user,
			this, &worker_node::request_update_user,
			Qt::DirectConnection);
	connect(m_p_tcp_thread, &tcp_thread::got_request_groups,
			this, &worker_node::request_user_groups,
			Qt::DirectConnection);
	connect(m_p_tcp_thread, &tcp_thread::got_request_account,
			this, &worker_node::request_account_info,
			Qt::DirectConnection);
	connect(m_p_tcp_thread, &tcp_thread::got_delete_group,
			this, &worker_node::request_delete_group,
			Qt::DirectConnection);
	connect(m_p_tcp_thread, &tcp_thread::got_list_group_users,
			this, &worker_node::request_group_users,
			Qt::DirectConnection);
	connect(m_p_tcp_thread, &tcp_thread::got_create_user_event,
			this, &worker_node::request_personal_event,
			Qt::DirectConnection);
	connect(m_p_tcp_thread, &tcp_thread::got_create_group_event,
			this, &worker_node::request_group_event,
			Qt::DirectConnection);
	connect(m_p_tcp_thread, &tcp_thread::got_reset_password,
			this, &worker_node::request_reset_password,
			Qt::DirectConnection);
	connect(m_p_tcp_thread, &tcp_thread::got_request_events,
			this, &worker_node::request_user_events,
			Qt::DirectConnection);
	connect(m_p_tcp_thread, &tcp_thread::got_request_month_events,
			this, &worker_node::request_personal_month_events,
			Qt::DirectConnection);
	connect(m_p_tcp_thread, &tcp_thread::got_request_group_events,
			this, &worker_node::request_group_events,
			Qt::DirectConnection);
	connect(m_p_tcp_thread, &tcp_thread::got_request_group_month_events,
			this, &worker_node::request_group_month_events,
			Qt::DirectConnection);
	connect(m_p_tcp_thread, &tcp_thread::got_create_friendship,
			this, &worker_node::request_create_friendship,
			Qt::DirectConnection);
	connect(m_p_tcp_thread, &tcp_thread::got_accept_friend,
			this, &worker_node::request_accept_friend,
			Qt::DirectConnection);
	connect(m_p_tcp_thread, &tcp_thread::got_request_friends,
			this, &worker_node::request_friends,
			Qt::DirectConnection);
	connect(m_p_tcp_thread, &tcp_thread::got_delete_friend,
			this, &worker_node::request_delete_friend,
			Qt::DirectConnection);
	connect(m_p_tcp_thread, &tcp_thread::got_friend_requests,
			this, &worker_node::request_friend_requests,
			Qt::DirectConnection);
	connect(m_p_tcp_thread, &tcp_thread::got_absent,
			this, &worker_node::request_absent,
			Qt::DirectConnection);
	connect(m_p_tcp_thread, &tcp_thread::got_present,
			this, &worker_node::request_present,
			Qt::DirectConnection);
	connect(m_p_tcp_thread, &tcp_thread::got_suggest_user_time,
			this, &worker_node::request_suggest_user_times,
			Qt::DirectConnection);
	connect(m_p_tcp_thread, &tcp_thread::dropped_client,
			this, &worker_node::handle_client_disconnect,
			Qt::DirectConnection);
	connect(m_p_tcp_thread, &tcp_thread::got_suggest_group_times,
			this, &worker_node::request_suggest_group_times,
			Qt::DirectConnection);
    /* start the thread */
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
			delete pSocket; read.clear();
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
		} state = connection_state::CONNECT_TO_MASTER;
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
 * Handle an unexpected client disconnect.
 */
void worker_node::handle_client_disconnect()
{
	m_p_mutex->lock();
	served_client = true;
	m_p_mutex->unlock();
}

/**
 * @brief Insert a group into the database.
 *
 * @param group_name Name of the group to create.
 * @return True if the insertion succeeded.
 */
bool worker_node::insert_group(const QString & group_name) {
	if(!m_db.open()) {
		std::cerr<<"Error! Failed to open database connection!"<<std::endl;
		return false;
	} else if (!group_name.size()) return false;

	QSqlQuery query(m_db); 
	query.prepare("CALL AddGroup(?, @success)");
	query.bindValue(0, group_name);	
	
	if(!query.exec()) {
		std::cerr<<"Query Failed to execute!"<<std::endl;
		std::cerr<<"query: \""<<query.lastQuery().toStdString()<<"\""<<std::endl;	
		throw std::invalid_argument("something failed during procedure call");
		return false;
	} else if (!query.exec("SELECT @success")) {
		std::cerr<<"Query Failed to execute!"<<std::endl;
		std::cerr<<"query: \""<<query.lastQuery().toStdString()<<"\""<<std::endl;	
		throw std::invalid_argument("something failed during procedure call");
		return false;
	} query.next();

	return query.value(0).toBool();
}

/**
 * @brief Add a user to a group.
 *
 * @param user_name Name of the user to add to the group.
 * @param group_name Name of the group to which we are adding the user.
 * @return True if the insertion succeeded.
 */
bool worker_node::leave_group(const QString & user_name, const QString & group_name) {
	if(!m_db.open()) {
		std::cerr<<"Error! Failed to open database connection!"<<std::endl;
		return false;
	} else if (!group_name.size()) return false;

	QSqlQuery query(m_db); 
	query.prepare("CALL RemoveFromGroup(?, ?, @success)");
	query.bindValue(0, group_name);	
	query.bindValue(1, user_name);
	
	if(!query.exec()) {
		std::cerr<<"Query Failed to execute!"<<std::endl;
		std::cerr<<"query: \""<<query.lastQuery().toStdString()<<"\""<<std::endl;	
		throw std::invalid_argument("something failed during procedure call");
		return false;
	} else if (!query.exec("SELECT @success")) {
		std::cerr<<"Query Failed to execute!"<<std::endl;
		std::cerr<<"query: \""<<query.lastQuery().toStdString()<<"\""<<std::endl;	
		throw std::invalid_argument("something failed during procedure call");
		return false;
	} query.next();

	return query.value(0).toBool();
}

/** 
 * Check if a group exists.
 * 
 * @param _group Name of the group to check.
 * 
 * @return True if the supplied group exists.
 */
bool worker_node::group_exists(const QString & _group) {
	if(!m_db.open()) {
		std::cerr<<"Error! Failed to open database connection!"<<std::endl;
		return false;
	} else if (!_group.size()) return false;

	QSqlQuery query(m_db); 
	query.prepare("SELECT count(*) FROM groups WHERE group_name = ?");
	query.bindValue(0, _group);
	
	if(!query.exec()) {
		std::cerr<<"Query Failed to execute!"<<std::endl;
		std::cerr<<"query: \""<<query.lastQuery().toStdString()<<"\""<<std::endl;	
		throw std::invalid_argument("something failed during procedure call");
		return false;
	} query.next();

	return query.value(0).toBool();
}

/**
 * @brief Delete a group using stored procedure.
 *
 * @param group_name Name of the group to remove.
 * @return True if the removal succeeded.
 */
bool worker_node::remove_group(const QString & group_name) {
	if(!m_db.open()) {
		std::cerr<<"Error! Failed to open database connection!"<<std::endl;
		return false;
	} else if (!group_name.size()) return false;

	QSqlQuery query(m_db); 
	query.prepare("CALL RemoveGroup(?, @success)");
	query.bindValue(0, group_name);	
	
	if(!query.exec()) {
		std::cerr<<"Query Failed to execute!"<<std::endl;
		std::cerr<<"query: \""<<query.lastQuery().toStdString()<<"\""<<std::endl;	
		throw std::invalid_argument("something failed during procedure call");
		return false;
	} else if (!query.exec("SELECT @success")) {
		std::cerr<<"Query Failed to execute!"<<std::endl;
		std::cerr<<"query: \""<<query.lastQuery().toStdString()<<"\""<<std::endl;	
		throw std::invalid_argument("something failed during procedure call");
		return false;
	} query.next();

	return query.value(0).toBool();
}

/**
 * @brief Add a user to a group.
 *
 * @param user_name Name of the user to add to the group.
 * @param group_name Name of the group to which we are adding the user.
 * @return True if the insertion succeeded.
 */
bool worker_node::join_group(const QString & user_name, const QString & group_name) {
	if(!m_db.open()) {
		std::cerr<<"Error! Failed to open database connection!"<<std::endl;
		return false;
	} else if (!group_name.size()) return false;

	QSqlQuery query(m_db); 
	query.prepare("CALL AddUserToGroup(?, ?, @success)");
	query.bindValue(0, group_name);	
	query.bindValue(1, user_name);
	
	if(!query.exec()) {
		std::cerr<<"Query Failed to execute!"<<std::endl;
		std::cerr<<"query: \""<<query.lastQuery().toStdString()<<"\""<<std::endl;	
		throw std::invalid_argument("something failed during procedure call");
		return false;
	} else if (!query.exec("SELECT @success")) {
		std::cerr<<"Query Failed to execute!"<<std::endl;
		std::cerr<<"query: \""<<query.lastQuery().toStdString()<<"\""<<std::endl;	
		throw std::invalid_argument("something failed during procedure call");
		return false;
	} query.next();

	return query.value(0).toBool();
}

/**
 * @brief Get a user's personal events
 *
 * @param group Group name.
 * @return True if the list was retrieved
 */
bool worker_node::list_user_events(const QString & owner,
								   const QString & start_date,
								   const QString & end_date,
								   QString * _msg)
{
	if(!m_db.open()) {
		std::cerr<<"Error! Failed to open database connection!"<<std::endl;
		return false;
	} else if (!owner.size()) return false;

	QSqlQuery query(m_db); 
	QString query_text = QString("SELECT schedule_item.date, schedule_item.start_time, "
								 "schedule_item.duration, schedule_item.location, "
								 "schedule_item.event_name FROM schedule_item, schedules "
								 "WHERE schedules.owner = '") + owner + "'"
		+ "AND schedule_item.date >= '" + start_date + "'"
		+ "AND schedule_item.date < '" + end_date +
		"' AND schedule_item.schedule_id = schedules.schedule_id;";
	query.prepare(query_text);
	
	if(!query.exec()) {
		std::cerr<<"Query Failed to execute!"<<std::endl;
		std::cerr<<"query: \""<<query.lastQuery().toStdString()<<"\""<<std::endl;	
		throw std::invalid_argument("failed to query the user's groups");
		return false;
	} else if (!query.size()) {
		*_msg += "\n";
		return true;
	}

	for (; query.next();) {
		*_msg += query.value(0).toString() + ":::"
			+ query.value(1).toString() + ":::"
			+ query.value(2).toString() + ":::"
			+ query.value(3).toString() + ":::"
			+ query.value(4).toString() + "\n";
	}
	return true;
}

QSet<calendar_event> worker_node::suggest_event_times(const QString & owner,
													  const QString & deadline_date,
													  const QString & deadline_time,
													  const QString & duration)
{
	/* buckle the fuck up */
	if(!m_db.open()) {
		std::cerr<<"Error! Failed to open database connection!"<<std::endl;
		throw std::invalid_argument("failed to find the database");
		QSet<calendar_event> rip; return rip;
	} else if (!owner.size()) {
		throw std::invalid_argument("empty owner string");
		QSet<calendar_event> rip; return rip;
	} QSet<calendar_event> times;

	/* what day even is it? */
	QString start_date = QDateTime::currentDateTime().toString("yyyy-M-d");
	
	QSqlQuery query(m_db);
	QString query_text = QString("SELECT schedule_item.date, schedule_item.start_time, "
								 "schedule_item.duration FROM schedule_item, schedules "
								 "WHERE schedules.owner = '") + owner + "' "
		+ "AND schedule_item.date >= '" + start_date + "' "
		+ "AND schedule_item.date <= '" + deadline_date + "' "
		+ "AND schedule_item.schedule_id = schedules.schedule_id;";
	query.prepare(query_text);

	/* I suppose we can just as for those days then? */
	if(!query.exec()) {
		std::cerr<<"Query Failed to execute!"<<std::endl;
		std::cerr<<"query: \""<<query.lastQuery().toStdString()<<"\""<<std::endl;	
		throw std::invalid_argument("failed to query the user's groups");
		QSet<calendar_event> rip; return rip;
	}

	/* iterate through the returned dates, getting fucked accordingly. */
	QSet<calendar_event> events, scheduled_times;
	/* push the current date */
	calendar_event current_time;
	current_time.date = QDateTime::currentDateTime().date();
	current_time.time = QDateTime::currentDateTime().time();
	current_time.duration = 0;
	events.insert(current_time);

	for (; query.next();) {
		calendar_event curr;
		curr.date = query.value(0).toDate();
		curr.time = query.value(1).toTime();

		QTime temp = query.value(2).toTime();
		/* extract minute duration from time */
		curr.duration = temp.hour() * 60 + temp.minute();
		events.insert(curr);
	} /* add the deadline */
	calendar_event deadline;
	deadline.date = QDate::fromString(deadline_date, "yyyy-M-d");
	deadline.time = QTime::fromString(deadline_time, "hh:mm");
	/* we don't care about the duration of the deadline */
	events.insert(deadline); int len = duration.toInt();

	/**
	 * @todo verify the deadline is not occupied,
	 * and, if it is, backtrack to the first
	 * free slot that can hold our event.
	 */

	QList<calendar_event> temp = events.toList();
	/* find the working times */
	for (int x = 0; x < events.size() - 1; ++x) {
		bool ok;
		calendar_event c = schedule_between(temp, x, x + 1, len, &ok);

		if (!ok) continue; /* don't insert if you didn't find a time */

		/**
		 * @todo check that the hour is not absurd.
		 */

		times.insert(c);
	} return times;
}

bool worker_node::is_valid_for_user(const QString & owner,
									const calendar_event & event)
{
	if(!m_db.open()) {
		std::cerr<<"Error! Failed to open database connection!"<<std::endl;
		throw std::invalid_argument("failed to find the database");
		return false;
	} else if (!owner.size()) {
		throw std::invalid_argument("empty owner string");
		return false;
	}

	/* what day even is it? */
	QString start_date = QDateTime::currentDateTime().toString("yyyy-M-d");
	
	QSqlQuery query(m_db);
	/**
	 * It might be wise to have an end date, but... I mean...
	 * I don't plan that far ahead... So... We'll wait for the report ;)
	 */
	QString query_text = QString("SELECT schedule_item.date, schedule_item.start_time, "
								 "schedule_item.duration FROM schedule_item, schedules "
								 "WHERE schedules.owner = '") + owner + "' "
		+ "AND schedule_item.date >= '" + start_date + "' "
		+ "AND schedule_item.schedule_id = schedules.schedule_id;";
	query.prepare(query_text);

	if(!query.exec()) {
		std::cerr<<"Query Failed to execute!"<<std::endl;
		std::cerr<<"query: \""<<query.lastQuery().toStdString()<<"\""<<std::endl;	
		throw std::invalid_argument("failed to query the user's groups");
		return false;
	}

	for (; query.next();) {
		calendar_event curr;
		curr.date = query.value(0).toDate();
		curr.time = query.value(1).toTime();

		QTime temp = query.value(2).toTime();
		/* extract minute duration from time */
		curr.duration = temp.hour() * 60 + temp.minute();

		/* return false if events overlap */
		if (check_overlap(event, curr)) return false;
	} return true;
}

bool worker_node::suggest_user_events(const QString & owner,
									  const QString & deadline_date,
									  const QString & deadline_time,
									  const QString & duration,
									  QString * _msg)
{
	QSet<calendar_event> suggestions;
	suggestions = suggest_event_times(owner, deadline_date,
									  deadline_time, duration);
    /* write back the request */
	if (!suggestions.size()) {
		*_msg = "\n";
		return true;
	}

	QList<calendar_event> times = suggestions.values();
	for (int x = 0; x < times.size(); ++x) {
		*_msg += times[x].date.toString("yyyy-M-d") + ":::"
			+ times[x].time.toString("hh:mm") + "\n";		
	} return true;
}									  

bool worker_node::suggest_group_events(const QString & owner,
									   const QString & deadline_date,
									   const QString & deadline_time,
									   const QString & duration,
									   QString * _msg)
{	
	QSet<calendar_event> group_times;
	QString * p_users; /* @todo delete me! */

	/**
	 * First, obtain a list of all users in the group.
	 * Should probably just do the damn query, but...
	 * I already wrote it down there and I'm lazy.
	 */
	if (!list_group_users(owner, p_users = new QString())) {
		/* if this fails, something is pretty messed up... */
		*_msg = "Please tell Hunter that\n"
			"the thing he thought wouldn't\n"
			"happen happened...\n";
		return true;
	}

	QStringList users = p_users->split("\n");
	QString empty = ""; users.removeAll(empty);

	/* free p_users, as it is no longer needed */
	delete p_users;
	
	for (int x = 0; x < users.size(); ++x) {
		/* grab the current user's events */
		QSet<calendar_event> suggestions;
		suggestions = suggest_event_times(users[x], deadline_date,
										  deadline_time, duration);
		/**
		 * Union the user's free times with the current
		 * group pool. We don't intersect here, since
		 * the suggested times we received for each user
		 * is not a complete list... It's only a mere
		 * approximation.
		 *
		 * P.S. Why the fuck is "|=" the union operator?
		 * Also, why is the union function called unite?
		 *
		 * Come on, Qt...
		 */
		group_times |= suggestions;
	}

	/**
	 * Next we iterate through the group times set, removing
	 * times that do not work for ALL group members.
	 */
	QList<calendar_event> temp = group_times.toList();
	for (int x = 0; x < users.size(); ++x) {
		for (int y = 0; y < temp.size(); ++y) {
			if (!is_valid_for_user(users[x], temp[y])) group_times -= temp[y];
		}
	}
	
    /* write back the request */
	if (!group_times.size()) {
		*_msg = "\n";
		return true;
	}

	/* return the list of times that work for everyone */
	QSet<calendar_event>::iterator i;
	for (i = group_times.begin(); i != group_times.end(); ++i) {
		*_msg += i->date.toString("yyyy-M-d") + ":::"
			+ i->time.toString("hh:mm") + "\n";		
	} return true;
}									  

bool worker_node::list_user_month_events(const QString & owner,
										 const quint8 & month,
										 const quint16 & year,
										 QString * _msg)
{
	if(!m_db.open()) {
		std::cerr<<"Error! Failed to open database connection!"<<std::endl;
		return false;
	} else if (!owner.size()) return false;
	QString start_date = QString().setNum(year) + "-" + QString().setNum(month)+"-01";
	QString end_date = QString().setNum((month == 12 ? year + 1 : year)) +
		"-" + QString().setNum((month == 12 ? 1 : (month + 1)))+"-01";
	QSqlQuery query(m_db); 
	QString query_text = QString("SELECT schedule_item.date FROM schedule_item, schedules "
								 "WHERE schedules.owner = '") + owner + "'"
		+ "AND schedule_item.date >= '" + start_date + "'"
		+ "AND schedule_item.date < '" + end_date +
		"' AND schedule_item.schedule_id = schedules.schedule_id;";
	query.prepare(query_text);
	
	if(!query.exec()) {
		std::cerr<<"Query Failed to execute!"<<std::endl;
		std::cerr<<"query: \""<<query.lastQuery().toStdString()<<"\""<<std::endl;	
		throw std::invalid_argument("failed to query the user's groups");
		return false;
	} else if (!query.size()) {
		*_msg = "0\n";
		return true;
	} int number = 0;

	QStringList list;
	for (; query.next();) {
		list = query.value(0).toString().split("-");
		number = (number | (1 << (list[2].toInt())));
	} _msg->setNum(number); *(_msg) += "\n";
	return true;
}

/**
 * @brief Get a user's groups.
 *
 * @param user User name
 * @return True if the list was retrieved
 */
bool worker_node::list_groups(const QString & user_name, QString * _msg) {
	if(!m_db.open()) {
		std::cerr<<"Error! Failed to open database connection!"<<std::endl;
		return false;
	} else if (!user_name.size()) return false;

	QSqlQuery query(m_db); 
	query.prepare("SELECT groups.group_name FROM groups, users,"
				  " user_group_relation WHERE users.user_name = ? "
				  "AND user_group_relation.user_id = users.user_id "
				  "AND user_group_relation.group_id = groups.group_id");
	query.bindValue(0, user_name);
	
	if(!query.exec()) {
		std::cerr<<"Query Failed to execute!"<<std::endl;
		std::cerr<<"query: \""<<query.lastQuery().toStdString()<<"\""<<std::endl;	
		throw std::invalid_argument("failed to query the user's groups");
		return false;
	} else if (!query.size()) {
		*_msg += "\n";
		return true;
	}

	for (; query.next();) *_msg += query.value(0).toString() + "\n";
	return true;
}

/**
 * @brief Get the users in a group.
 *
 * @param group Group name.
 * @return True if the list was retrieved
 */
bool worker_node::list_group_users(const QString & group_name, QString * _msg)
{
	if(!m_db.open()) {
		std::cerr<<"Error! Failed to open database connection!"<<std::endl;
		return false;
	} else if (!group_name.size()) return false;

	QSqlQuery query(m_db); 
	query.prepare("SELECT users.user_name FROM groups, users,"
				  " user_group_relation WHERE groups.group_name = ? "
				  "AND user_group_relation.user_id = users.user_id "
				  "AND user_group_relation.group_id = groups.group_id");
	query.bindValue(0, group_name);
	
	if(!query.exec()) {
		std::cerr<<"Query Failed to execute!"<<std::endl;
		std::cerr<<"query: \""<<query.lastQuery().toStdString()<<"\""<<std::endl;	
		throw std::invalid_argument("failed to query the user's groups");
		return false;
	} else if (!query.size()) {
		*_msg += "\n";
		return true;
	}

	for (; query.next();) *_msg += query.value(0).toString() + "\n";
	return true;
}

/**
 * @brief Get a user's account info.
 *
 * @param user User name
 * @return True if the list was retrieved
 */
bool worker_node::get_account_info(const QString & user_name, QString * _msg) {
	if(!m_db.open()) {
		std::cerr<<"Error! Failed to open database connection!"<<std::endl;
		return false;
	} else if (!user_name.size()) return false;

	QSqlQuery query(m_db); 
	query.prepare("SELECT email, cellphone FROM users WHERE user_name = ?");
	query.bindValue(0, user_name);
	
	if(!query.exec()) {
		std::cerr<<"Query Failed to execute!"<<std::endl;
		std::cerr<<"query: \""<<query.lastQuery().toStdString()<<"\""<<std::endl;	
		throw std::invalid_argument("failed to query the user's groups");
		return false;
	} else if (!query.size()) {
		*_msg += "\n";
		return true;
	} query.next();

	*_msg += query.value(0).toString() + ":::" + query.value(1).toString() + "\r\n";
	return true;
}

/**
 * @brief Update the user with this shit ton of parameters.
 *
 * @return True if the insertion succeeded.
 */
bool worker_node::update_user(const QString & _old_user, const QString & _old_pass,
							  const QString & _new_pass, const QString & _new_user,
							  const QString & _new_mail, const QString & _new_cell)
{
	if(!m_db.open()) {
		std::cerr<<"Error! Failed to open database connection!"<<std::endl;
		return false;
	} else if (!(_old_user.size() && _old_pass.size() && _new_pass.size()
				 && _new_user.size() && _new_mail.size())) return false;

	QSqlQuery query(m_db); 
	query.prepare("UPDATE users SET user_name = ?, passwd = ?,"
				  "email = ?, cellphone = ? WHERE user_name = ? AND passwd = ?");
	query.bindValue(0, _new_user); query.bindValue(1, _new_pass);
	query.bindValue(2, _new_mail); query.bindValue(3, _new_cell);
	query.bindValue(4, _old_user); query.bindValue(5, _old_pass);
	
	if(!query.exec()) {
		std::cerr<<"Query Failed to execute!"<<std::endl;
		std::cerr<<"query: \""<<query.lastQuery().toStdString()<<"\""<<std::endl;	
		throw std::invalid_argument("something failed during procedure call");
		return false;
	} return true;
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
	} else if (query->size()) return delete query, true;
	return delete query, false;
}

bool worker_node::reset_password(QString & _p_user,
								 QString & _p_email,
								 QString & _p_new_psswd)
{
	if(!m_db.open()) {
		std::cerr<<"Error! Failed to open database connection!"<<std::endl;
		return false;
	} else if (!(_p_user.size()
				 && _p_email.size()
				 && _p_new_psswd.size())) return false;
	QSqlQuery q(m_db);
	q.prepare("SELECT DISTINCT email FROM users WHERE user_name = ?");
	q.bindValue(0, _p_user);

	if(!q.exec()) {
		std::cerr<<"Query Failed to execute!"<<std::endl;
		std::cerr<<"query: \""<<q.lastQuery().toStdString()<<"\""<<std::endl;	
		throw std::invalid_argument("something failed during procedure call");
		return false;
	} else if (!q.size()) return false;

	q.next();
	
	register int email_col = q.record().indexOf("email");
	QVariant _email;

	if (email_col != -1) _email = q.value(email_col);
	else throw std::invalid_argument("No email column returned");
	QString email = _email.toString();
	if(QString::compare(_p_email,email)!=0) return false;

	QSqlQuery query(m_db); 
	query.prepare("UPDATE users SET passwd = ? "
				  "WHERE user_name = ? AND email = ?");
	query.bindValue(0, _p_new_psswd); query.bindValue(1, _p_user);
	query.bindValue(2, _p_email); 
	
	if(!query.exec()) {
		std::cerr<<"Query Failed to execute!"<<std::endl;
		std::cerr<<"query: \""<<query.lastQuery().toStdString()<<"\""<<std::endl;	
		throw std::invalid_argument("something failed during procedure call");
		return false;
	} return true;	
}

bool worker_node::select_schedule_id(user & u)
{
	if(!m_db.open()) {
		std::cerr<<"Error! Failed to open database connection!"<<std::endl;
		return false;
	}

	QSqlQuery * query = new QSqlQuery(m_db);
   
	QString schedule_select = "SELECT DISTINCT schedule_id FROM schedules WHERE ";
	schedule_select += "owner = '" + u.get_username() + "';";

	if (!query->exec(schedule_select)) {
		std::cerr<<"Query failed to execute!"<<std::endl;
		std::cerr<<"query: \""<<query->lastQuery().toStdString()<<"\""<<std::endl;
		schedule_select = "Something failed in select query:\n" + schedule_select;
		std::string str = schedule_select.toStdString(); delete query;
		throw std::invalid_argument(str);
		return false;
	} else if (!query->size()) return delete query, false;

	/* now extract the schedule id and set in our referenced object */
	register int sched_id_col = query->record().indexOf("schedule_id");
	query->next();
	if (sched_id_col != -1) u.set_schedule_id(query->value(sched_id_col).toString());
	else throw std::invalid_argument("No schedule_id column returned");
	return delete query, true;
}

bool worker_node::select_user(user & u) {
    if(!m_db.open()) {
		std::cerr<<"Error! Failed to open database connection!"<<std::endl;
		return false;
	}

    QSqlQuery * query = new QSqlQuery(m_db);
   
	QString user_stuff = "SELECT DISTINCT user_id, schedule_id, email, cellphone FROM users WHERE ";
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

bool worker_node::create_personal_event(
	const QString & user,
	const QString & date,
	const QString & start,
	const QString & duration,
	const QString & location,
	const QString & timezone_offset,
	const QString & name,
	const QString & immutable
	)
{
	if(!m_db.open()) {
		std::cerr<<"Error! Failed to open database connection!"<<std::endl;
		return false;
	}

	QSqlQuery query(m_db); 
	query.prepare("CALL AddPersonalEvent(?, ?, ?, ?, ?, ?, ?, ?, @success)");

	/* convert minutes to a time */
	bool ok; int duration_int = duration.toInt(&ok);

	if (!ok) {
		std::cerr<<"Duration is not an integer!"<<std::endl;
		throw std::invalid_argument("invalid duration passed");
		return false;
	} QTime time(duration_int / 60, duration_int % 60);

	int offset = timezone_offset.toInt(&ok);
	if (!ok) {
		std::cerr<<"Timezone offset is not an integer!"<<std::endl;
		throw std::invalid_argument("invalid timezone offset passed");
		return false;
	}
	
	query.bindValue(0, user); query.bindValue(1, date); query.bindValue(2, start);
	query.bindValue(3, time.toString("hh:mm:ss")); query.bindValue(4, location);
	query.bindValue(5, offset); query.bindValue(6, name);
	query.bindValue(7, immutable);

	if(!query.exec()) {
		std::cerr<<"Query Failed to execute!"<<std::endl;
		std::cerr<<"query: \""<<query.lastQuery().toStdString()<<"\""<<std::endl;	
		throw std::invalid_argument("something failed during procedure call");
		return false;
	} else if (!query.exec("SELECT @success")) {
		std::cerr<<"Query Failed to execute!"<<std::endl;
		std::cerr<<"query: \""<<query.lastQuery().toStdString()<<"\""<<std::endl;	
		throw std::invalid_argument("something failed during procedure call");
		return false;
	} query.next();

	return query.value(0).toBool();
}

/** 
 * Check if a user is a member of a group.
 * 
 * @param user The user in question.
 * @param group Group in question.
 * 
 * @return True if the user is in the group.
 */
bool worker_node::user_in_group(
	const QString & user,
	const QString & group
	)
{
	if(!m_db.open()) {
		std::cerr<<"Error! Failed to open database connection!"<<std::endl;
		return false;
	}

	QSqlQuery query(m_db); 
	query.prepare("SELECT count(*) FROM users, user_group_relation, "
				  "groups WHERE users.user_id = user_group_relation.user_id "
				  "AND groups.group_id = user_group_relation.group_id "
				  "AND users.user_name = ? "
				  "AND groups.group_name = ?");	
	query.bindValue(0, user);     query.bindValue(1, group);
	
	if(!query.exec()) {
		std::cerr<<"Query Failed to execute!"<<std::endl;
		std::cerr<<"query: \""<<query.lastQuery().toStdString()<<"\""<<std::endl;	
		throw std::invalid_argument("something failed during procedure call");
		return false;
	} query.next();

	return query.value(0).toBool();
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

bool worker_node::delete_friend(const QString & _user,
								const QString & _friend) {
	if(!m_db.open()) {
		std::cerr<<"Error! Failed to open database connection!"<<std::endl;
		return false;
	} else if (_user.size()==0 || _friend.size()==0) return false;

	QSqlQuery query(m_db);
	QString txt = "CALL DeleteFriend(\'";
	txt+=_user + "\', \'" + _friend + "\', " + "@success);";
	std::cerr<<"txt = "<<txt.toStdString()<<std::endl;
	query.prepare(txt); 
	
	if(!query.exec()) {
		std::cerr<<"Query Failed to execute!"<<std::endl;
		std::cerr<<"query: \""<<query.lastQuery().toStdString()<<"\""<<std::endl;	
		throw std::invalid_argument("something failed during procedure call");
		return false;
	} else if (!query.exec("SELECT @success")) {
		std::cerr<<"Query Failed to execute!"<<std::endl;
		std::cerr<<"query: \""<<query.lastQuery().toStdString()<<"\""<<std::endl;	
		throw std::invalid_argument("something failed during procedure call");
		return false;
	} query.next();
			   
	return query.value(0).toBool();
}

bool worker_node::friends(const QString & _user, QString * _msg) {
    if(!m_db.open()) {
		std::cerr<<"Error! Failed to open database connection!"<<std::endl;
		return false;
	} else if (_user.size()==0) return false;

	QSqlQuery query(m_db);
	QString txt = "SELECT DISTINCT u2.user_name ";
	txt+="FROM users u1, users u2, user_friend_relation ";
	txt+="WHERE u1.user_name = '"+ _user +"' AND ";
	txt+="((user_friend_relation.user_id = u1.user_id AND ";
	txt+="user_friend_relation.friend_id = u2.user_id) OR ";
	txt+="(user_friend_relation.friend_id = u1.user_id AND ";
	txt+="user_friend_relation.user_id = u2.user_id)) AND ";
	txt+="u2.user_name != '" + _user + "' AND ";
	txt+="user_friend_relation.accepted = 1;";
   	std::cerr<<"txt = "<<txt.toStdString()<<std::endl;
	query.prepare(txt);   
	
	if(!query.exec()) {
		std::cerr<<"Query Failed to execute!"<<std::endl;
		std::cerr<<"query: \""<<query.lastQuery().toStdString()<<"\""<<std::endl;	
		throw std::invalid_argument("something failed during procedure call");
		return false;
	}

	for (; query.next();) *_msg += query.value(0).toString() + "\n";
	std::cerr<<"_msg = "<<_msg->toStdString()<<std::endl;
	return true;
}

bool worker_node::friend_requests(const QString & _user, QString * _msg) {
	if(!m_db.open()) {
		std::cerr<<"Error! Failed to open database connection!"<<std::endl;
		return false;
	} else if (_user.size()==0) return false;

	QSqlQuery query(m_db);
	QString txt = "SELECT DISTINCT u2.user_name FROM ";
	txt+="users u1, users u2, user_friend_relation ";
	txt+="WHERE u1.user_name = '" + _user + "' AND ";
	txt+="user_friend_relation.friend_id = u1.user_id AND ";
	txt+="user_friend_relation.user_id = u2.user_id AND ";
	txt+="u2.user_name != '" + _user + "' AND ";
	txt+="user_friend_relation.accepted = 0; ";
	std::cerr<<"txt = "<<txt.toStdString()<<std::endl;
	query.prepare(txt);   
	
	if(!query.exec()) {
		std::cerr<<"Query Failed to execute!"<<std::endl;
		std::cerr<<"query: \""<<query.lastQuery().toStdString()<<"\""<<std::endl;	
		throw std::invalid_argument("something failed during procedure call");
		return false;
	} 

	for (; query.next();) *_msg += query.value(0).toString() + "\n";
	std::cerr<<"_msg = "<<_msg->toStdString()<<std::endl;
	return true;
}

bool worker_node::accept_friend(const QString & _user, const QString & _friend) {
	if(!m_db.open()) {
		std::cerr<<"Error! Failed to open database connection!"<<std::endl;
		return false;
	} else if (_user.size()==0 || _friend.size()==0) return false;

	QSqlQuery query(m_db);
	QString txt = "CALL AcceptFriend(\'";
	txt+=_user + "\', \'" + _friend + "\', " + "@success);";
	std::cerr<<"txt = "<<txt.toStdString()<<std::endl;
	query.prepare(txt);   
	
	if(!query.exec()) {
		std::cerr<<"Query Failed to execute!"<<std::endl;
		std::cerr<<"query: \""<<query.lastQuery().toStdString()<<"\""<<std::endl;	
		throw std::invalid_argument("something failed during procedure call");
		return false;
	} else if (!query.exec("SELECT @success")) {
		std::cerr<<"Query Failed to execute!"<<std::endl;
		std::cerr<<"query: \""<<query.lastQuery().toStdString()<<"\""<<std::endl;	
		throw std::invalid_argument("something failed during procedure call");
		return false;
	} query.next();
			   
	return query.value(0).toBool();
}

/**
 * @brief Try to create a friendship
 *
 * @param _user username.
 * @param _password Encrypted password.
 * @param _friend username
 * @return True upon successful creation.
 */
bool worker_node::create_friendship(const QString & _user,
									const QString & _friend) {
	if(!m_db.open()) {
		std::cerr<<"Error! Failed to open database connection!"<<std::endl;
		return false;
	} if (_user.size()==0 || _friend.size()==0) {
		std::cerr<<"Error! Invalid request!"<<std::endl;
		return false;
	}

	QSqlQuery query(m_db);
	QString txt = "CALL AddFriend(\'";
	txt+=_user + "\', \'" + _friend + "\', " + "@success);";
	std::cerr<<"txt = "<<txt.toStdString()<<std::endl;
	query.prepare(txt);   
	
	if(!query.exec()) {
		std::cerr<<"Query Failed to execute!"<<std::endl;
		std::cerr<<"query: \""<<query.lastQuery().toStdString()<<"\""<<std::endl;	
		throw std::invalid_argument("something failed during procedure call");
		return false;
	} else if (!query.exec("SELECT @success")) {
		std::cerr<<"Query Failed to execute!"<<std::endl;
		std::cerr<<"query: \""<<query.lastQuery().toStdString()<<"\""<<std::endl;	
		throw std::invalid_argument("something failed during procedure call");
		return false;
	} query.next();

	return query.value(0).toBool();
}

bool worker_node::cleanup_group_insert()
{
	if(!m_db.open()) {
		std::cerr<<"Error! Failed to open database connection!"<<std::endl;
		return false;
	}

	/* now we remove the inserted */
	QString delete_group = "DELETE FROM groups WHERE group_name = 'billy group';";	
	QString delete_schedule_item = "DELETE FROM schedules WHERE owner = 'billy group';";
	/* @todo remove from group */
	/* QString delete_relation = "CALL RemoveFromGroup(?, ?, @success) */
	QSqlQuery * query = new QSqlQuery(m_db);
	query->prepare(delete_group);

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

bool worker_node::present(const QString & _user) {
	if(!m_db.open()) {
		std::cerr<<"Error! Failed to open database connection!"<<std::endl;
		return false;
	} else if (_user.size()==0) return false;

	QSqlQuery query(m_db);
    query.prepare("UPDATE users SET absent = 0 "
				  "WHERE user_name = ? ");
	query.bindValue(0, _user);   
	
	if(!query.exec()) {
		std::cerr<<"Query Failed to execute!"<<std::endl;
		std::cerr<<"query: \""<<query.lastQuery().toStdString()<<"\""<<std::endl;	
		throw std::invalid_argument("failed to query the user's groups");
		return false;
	} return true;
}

bool worker_node::absent(const QString & _user) {
	if(!m_db.open()) {
		std::cerr<<"Error! Failed to open database connection!"<<std::endl;
		return false;
	} else if (_user.size()==0) return false;

	QSqlQuery query(m_db);
    query.prepare("UPDATE users SET absent = 1 "
				  "WHERE user_name = ? ");
	query.bindValue(0, _user);   
	
	if(!query.exec()) {
		std::cerr<<"Query Failed to execute!"<<std::endl;
		std::cerr<<"query: \""<<query.lastQuery().toStdString()<<"\""<<std::endl;	
		throw std::invalid_argument("failed to query the user's groups");
		return false;
	} return true;
}

bool worker_node::cleanup_user_group_insert()
{
	if(!m_db.open()) {
		std::cerr<<"Error! Failed to open database connection!"<<std::endl;
		return false;
	}

	QSqlQuery query2(m_db); 
	query2.prepare("CALL RemoveFromGroup(?, ?, @success)");
	query2.bindValue(0, "billy group");
	query2.bindValue(1, "billy");
	
	if(!query2.exec()) {
		std::cerr<<"Query Failed to execute!"<<std::endl;
		std::cerr<<"query: \""<<query2.lastQuery().toStdString()<<"\""<<std::endl;	
		throw std::invalid_argument("something failed during procedure call");
		return false;
	} else if (!query2.exec("SELECT @success")) {
		std::cerr<<"Query Failed to execute!"<<std::endl;
		std::cerr<<"query: \""<<query2.lastQuery().toStdString()<<"\""<<std::endl;	
		throw std::invalid_argument("something failed during procedure call");
		return false;
	} query2.next();

	if (!query2.value(0).toBool()) return false;
	
	/* now we remove the inserted */
	QString delete_group = "DELETE FROM groups WHERE group_name = 'billy group';";	
	QString delete_schedule_item = "DELETE FROM schedules WHERE owner = 'billy group';";
	/* @todo remove from group */
	/* QString delete_relation = "CALL RemoveFromGroup(?, ?, @success) */
	QSqlQuery * query = new QSqlQuery(m_db);
	query->prepare(delete_group);

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

void worker_node::request_absent(QString * _p_text,
								 QTcpSocket * _p_socket) {
	std::cout<<"create friendship request: \""<<
		_p_text->toStdString()<<"\""<<std::endl;
	/* create a tcp_connection object */
	QString client_host = _p_socket->peerName();
	tcp_connection * p = new tcp_connection(client_host, _p_socket);
	
	/* split along ':' characters */
	QStringList separated = _p_text->split(":::");
	
	if (separated.size() < 2) {
		/* if there are not enough params, disconnect. */
		QString * msg = new QString("ERROR: INVALID REQUEST\r\n");
		m_p_mutex->lock();
		served_client = true;
		m_p_mutex->unlock();
		Q_EMIT(disconnect_client(p, msg));
		return;
	}

	QString _user = separated[0];
	QString _pass = separated[1];
	
	QString * msg;

	try {
		if (!try_login(_user, _pass)) {
			msg = new QString("ERROR: AUTHENTICATION FAILED\r\n");
			delete _p_text;
			m_p_mutex->lock();
			served_client = true;
			m_p_mutex->unlock();
			Q_EMIT(disconnect_client(p, msg));
			return;
		} else if (!absent(_user)) {
			msg = new QString("ERROR: USER NOT ABSENT\r\n");
			m_p_mutex->lock();
			served_client = true;
			m_p_mutex->unlock();
			Q_EMIT(disconnect_client(p, msg));
			delete _p_text;
			return;
		} else msg = new QString("OK\r\n");
	} catch ( ... ) {
		msg = new QString("ERROR: DB COMMUNICATION FAILED\r\n");		
	} Q_EMIT(disconnect_client(p, msg));
	m_p_mutex->lock();
	served_client = true;
	m_p_mutex->unlock();
	delete _p_text;
}

void worker_node::request_present(QString * _p_text,
								  QTcpSocket * _p_socket) {
	std::cout<<"create friendship request: \""<<
		_p_text->toStdString()<<"\""<<std::endl;
	/* create a tcp_connection object */
	QString client_host = _p_socket->peerName();
	tcp_connection * p = new tcp_connection(client_host, _p_socket);
	
	/* split along ':' characters */
	QStringList separated = _p_text->split(":::");
	
	if (separated.size() < 2) {
		/* if there are not enough params, disconnect. */
		QString * msg = new QString("ERROR: INVALID REQUEST\r\n");
		m_p_mutex->lock();
		served_client = true;
		m_p_mutex->unlock();
		Q_EMIT(disconnect_client(p, msg));
		return;
	}

	QString _user = separated[0];
	QString _pass = separated[1];
	
	QString * msg;

	try {
		if (!try_login(_user, _pass)) {
			msg = new QString("ERROR: AUTHENTICATION FAILED\r\n");
			delete _p_text;
			m_p_mutex->lock();
			served_client = true;
			m_p_mutex->unlock();
			Q_EMIT(disconnect_client(p, msg));
			return;
		} else if (!present(_user)) {
			msg = new QString("ERROR: USER NOT PRESENT\r\n");
			m_p_mutex->lock();
			served_client = true;
			m_p_mutex->unlock();
			Q_EMIT(disconnect_client(p, msg));
			delete _p_text;
			return;
		} else msg = new QString("OK\r\n");
	} catch ( ... ) {
		msg = new QString("ERROR: DB COMMUNICATION FAILED\r\n");		
	} Q_EMIT(disconnect_client(p, msg));
	m_p_mutex->lock();
	served_client = true;
	m_p_mutex->unlock();
	delete _p_text;
}

void worker_node::request_accept_friend(QString * _p_text,
										QTcpSocket * _p_socket) {
	std::cout<<"create friendship request: \""<<
		_p_text->toStdString()<<"\""<<std::endl;
	/* create a tcp_connection object */
	QString client_host = _p_socket->peerName();
	tcp_connection * p = new tcp_connection(client_host, _p_socket);
	
	/* split along ':' characters */
	QStringList separated = _p_text->split(":::");
	
	if (separated.size() < 3) {
		/* if there are not enough params, disconnect. */
		QString * msg = new QString("ERROR: INVALID REQUEST\r\n");
		m_p_mutex->lock();
		served_client = true;
		m_p_mutex->unlock();
		Q_EMIT(disconnect_client(p, msg));
		return;
	}

	QString _user = separated[0];
	QString _pass = separated[1];
	QString _friend = separated[2];
	
	QString * msg;

	try {
		if (!try_login(_user, _pass)) {
			msg = new QString("ERROR: AUTHENTICATION FAILED\r\n");
			delete _p_text;
			m_p_mutex->lock();
			served_client = true;
			m_p_mutex->unlock();
			Q_EMIT(disconnect_client(p, msg));
			return;
		} else if (!accept_friend(_user, _friend)) {
			msg = new QString("ERROR: FRIEND REQUEST DOES NOT EXIST\r\n");
			m_p_mutex->lock();
			served_client = true;
			m_p_mutex->unlock();
			Q_EMIT(disconnect_client(p, msg));
			delete _p_text;
			return;
		} else msg = new QString("OK\r\n");
	} catch ( ... ) {
		msg = new QString("ERROR: DB COMMUNICATION FAILED\r\n");		
	} Q_EMIT(disconnect_client(p, msg));
	m_p_mutex->lock();
	served_client = true;
	m_p_mutex->unlock();
	delete _p_text;
}

void worker_node::request_create_friendship(QString * _p_text,
											QTcpSocket * _p_socket) {
	std::cout<<"create friendship request: \""<<
		_p_text->toStdString()<<"\""<<std::endl;
	/* create a tcp_connection object */
	QString client_host = _p_socket->peerName();
	tcp_connection * p = new tcp_connection(client_host, _p_socket);
	
	/* split along ':' characters */
	QStringList separated = _p_text->split(":::");
	
	if (separated.size() < 3) {
		/* if there are not enough params, disconnect. */
		QString * msg = new QString("ERROR: INVALID REQUEST\r\n");
		m_p_mutex->lock();
		served_client = true;
		m_p_mutex->unlock();
		Q_EMIT(disconnect_client(p, msg));
		return;
	}

	QString _user = separated[0];
	QString _pass = separated[1];
	QString _friend = separated[2];
	
	QString * msg;

	try {
		if (!try_login(_user, _pass)) {
			msg = new QString("ERROR: AUTHENTICATION FAILED\r\n");
			delete _p_text;
			m_p_mutex->lock();
			served_client = true;
			m_p_mutex->unlock();
			Q_EMIT(disconnect_client(p, msg));
			return;
		} else if (!create_friendship(_user, _friend)) {
			msg = new QString("ERROR: FRIEND DOES NOT EXIST\r\n");
			m_p_mutex->lock();
			served_client = true;
			m_p_mutex->unlock();
			Q_EMIT(disconnect_client(p, msg));
			delete _p_text;
			return;
		} else msg = new QString("OK\r\n");
	} catch ( ... ) {
		msg = new QString("ERROR: DB COMMUNICATION FAILED\r\n");		
	} Q_EMIT(disconnect_client(p, msg));
	m_p_mutex->lock();
	served_client = true;
	m_p_mutex->unlock();
	delete _p_text;
}

void worker_node::request_friends(QString * _p_text,
								  QTcpSocket * _p_socket) {
	std::cout<<"friends request: \""<<
		_p_text->toStdString()<<"\""<<std::endl;
	/* create a tcp_connection object */
	QString client_host = _p_socket->peerName();
	tcp_connection * p = new tcp_connection(client_host, _p_socket);
	
	/* split along ':' characters */
	QStringList separated = _p_text->split(":::");
	
	if (separated.size() < 2) {
		/* if there are not enough params, disconnect. */
		QString * msg = new QString("ERROR: INVALID REQUEST\r\n");
		m_p_mutex->lock();
		served_client = true;
		m_p_mutex->unlock();
		Q_EMIT(disconnect_client(p, msg));
		return;
	}

	QString _user = separated[0];
	QString _pass = separated[1];
	
	QString * msg;

	try {
		if (!try_login(_user, _pass)) {
			msg = new QString("ERROR: AUTHENTICATION FAILED\r\n");
			delete _p_text;
			m_p_mutex->lock();
			served_client = true;
			m_p_mutex->unlock();
			Q_EMIT(disconnect_client(p, msg));
			return;
		} else if (!friends(_user, msg = new QString())) {
			msg = new QString("ERROR: USER DOES NOT EXIST\r\n");
			m_p_mutex->lock();
			served_client = true;
			m_p_mutex->unlock();
			Q_EMIT(disconnect_client(p, msg));
			delete _p_text;
			return;
		} 
	} catch ( ... ) {
		msg = new QString("ERROR: DB COMMUNICATION FAILED\r\n");		
	} Q_EMIT(disconnect_client(p, msg));
	m_p_mutex->lock();
	served_client = true;
	m_p_mutex->unlock();
	delete _p_text;
}

void worker_node::request_delete_friend(QString * _p_text,
										QTcpSocket * _p_socket) {
	std::cout<<"delete friend request: \""<<
		_p_text->toStdString()<<"\""<<std::endl;
	/* create a tcp_connection object */
	QString client_host = _p_socket->peerName();
	tcp_connection * p = new tcp_connection(client_host, _p_socket);
	
	/* split along ':' characters */
	QStringList separated = _p_text->split(":::");
	
	if (separated.size() < 3) {
		/* if there are not enough params, disconnect. */
		QString * msg = new QString("ERROR: INVALID REQUEST\r\n");
		m_p_mutex->lock();
		served_client = true;
		m_p_mutex->unlock();
		Q_EMIT(disconnect_client(p, msg));
		return;
	}

	QString _user = separated[0];
	QString _pass = separated[1];
	QString _friend = separated[2];
	
	QString * msg;

	try {
		if (!try_login(_user, _pass)) {
			msg = new QString("ERROR: AUTHENTICATION FAILED\r\n");
			delete _p_text;
			m_p_mutex->lock();
			served_client = true;
			m_p_mutex->unlock();
			Q_EMIT(disconnect_client(p, msg));
			return;
		} else if (!delete_friend(_user, _friend)) {
			msg = new QString("ERROR: FRIEND DOES NOT EXIST\r\n");
			m_p_mutex->lock();
			served_client = true;
			m_p_mutex->unlock();
			Q_EMIT(disconnect_client(p, msg));
			delete _p_text;
			return;
		} else msg = new QString("OK\r\n");
	} catch ( ... ) {
		msg = new QString("ERROR: DB COMMUNICATION FAILED\r\n");		
	} Q_EMIT(disconnect_client(p, msg));
	m_p_mutex->lock();
	served_client = true;
	m_p_mutex->unlock();
	delete _p_text;
}

void worker_node::request_friend_requests(QString * _p_text,
										  QTcpSocket * _p_socket) {
	std::cout<<"friend requests: \""<<
		_p_text->toStdString()<<"\""<<std::endl;
	/* create a tcp_connection object */
	QString client_host = _p_socket->peerName();
	tcp_connection * p = new tcp_connection(client_host, _p_socket);
	
	/* split along ':' characters */
	QStringList separated = _p_text->split(":::");
	
	if (separated.size() < 2) {
		/* if there are not enough params, disconnect. */
		QString * msg = new QString("ERROR: INVALID REQUEST\r\n");
		m_p_mutex->lock();
		served_client = true;
		m_p_mutex->unlock();
		Q_EMIT(disconnect_client(p, msg));
		return;
	}

	QString _user = separated[0];
	QString _pass = separated[1];
	
	QString * msg;

	try {
		if (!try_login(_user, _pass)) {
			msg = new QString("ERROR: AUTHENTICATION FAILED\r\n");
			delete _p_text;
			m_p_mutex->lock();
			served_client = true;
			m_p_mutex->unlock();
			Q_EMIT(disconnect_client(p, msg));
			return;
		} else if (!friend_requests(_user, msg = new QString())) {
			msg = new QString("ERROR: USER DOES NOT EXIST\r\n");
			m_p_mutex->lock();
			served_client = true;
			m_p_mutex->unlock();
			Q_EMIT(disconnect_client(p, msg));
			delete _p_text;
			return;
		} 
	} catch ( ... ) {
		msg = new QString("ERROR: DB COMMUNICATION FAILED\r\n");		
	} Q_EMIT(disconnect_client(p, msg));
	m_p_mutex->lock();
	served_client = true;
	m_p_mutex->unlock();
	delete _p_text;
}

void worker_node::request_create_account(QString * _p_text,
										 QTcpSocket * _p_socket)
{
	std::cout<<"create account request: \""<<_p_text->toStdString()<<"\""<<std::endl;
	/* create a tcp_connection object */
	QString client_host = _p_socket->peerName();
	tcp_connection * p = new tcp_connection(client_host, _p_socket);
	
	/* split along ':' characters */
	QStringList separated = _p_text->split(":::");
	
	if (separated.size() < 3) {
		/* if there are not enough params, disconnect. */
		QString * msg = new QString("ERROR: INVALID REQUEST\r\n");
		m_p_mutex->lock();
		served_client = true;
		m_p_mutex->unlock();
		Q_EMIT(disconnect_client(p, msg));
		return;
	}

	QString _user = separated[0];
	QString _pass = separated[1];
	QString _mail = separated[2];
	QString _cell = (separated.size() > 3) ? separated[3] : "";

	if (username_exists(_user)) {
		/* the user exists. Invalid request. */
		QString * msg = new QString("ERROR: EXISTING USER\r\n");
		m_p_mutex->lock();
		served_client = true;
		m_p_mutex->unlock();		
		Q_EMIT(disconnect_client(p, msg));
		return;
	}
	QString * msg;
	/* try to insert the user */
	try {
		
		if (!try_create(_user, _pass, _mail)) {
			msg = new QString("ERROR: DB INSERT FAILED\r\n");
			delete _p_text;
			m_p_mutex->lock();
			served_client = true;
			m_p_mutex->unlock();
			return;
		} else msg = new QString("OK\r\n");
	} catch ( ... ) {
		msg = new QString("ERROR: DB INSERT FAILED\r\n");
	} Q_EMIT(disconnect_client(p, msg));
	m_p_mutex->lock();
	served_client = true;
	m_p_mutex->unlock();
	delete _p_text;
}

void worker_node::request_reset_password(QString * _p_text, QTcpSocket * _p_socket)
{
	std::cout<<"request reset password: \""<<_p_text->toStdString()<<"\""<<std::endl;
	/* create a tcp_connection object */
	QString client_host = _p_socket->peerName();
	tcp_connection * p = new tcp_connection(client_host, _p_socket);

	/* split along ':' characters */
	QStringList separated= _p_text->split(":::");

	if (separated.size() < 3) {
		/* invalid params => disconnect */
		QString * msg = new QString("ERROR: INVALID REQUEST\r\n");
		m_p_mutex->lock();
		served_client = true;
		m_p_mutex->unlock();
		Q_EMIT(disconnect_client(p, msg));
		return;
	}

	QString _user = separated[0];
	QString _email = separated[1];
	QString  _psswd = separated[2];

	QString * msg;

	try {
		if (!username_exists(_user)) {
			std::cerr<<"Authentication Error"<<std::endl;
			msg = new QString("ERROR: USER DOES NOT EXIST\r\n");
			Q_EMIT(disconnect_client(p, msg));
			delete _p_text;
			m_p_mutex->lock();
			served_client = true;
			m_p_mutex->unlock();
			return;
		} else if (!reset_password(_user, _email, _psswd)) {
			msg = new QString("ERROR: WRONG USERNAME & EMAIL COMBO\r\n");
			m_p_mutex->lock();
			served_client = true;
			m_p_mutex->unlock();
			Q_EMIT(disconnect_client(p, msg));
			delete _p_text;
			return;
		} else msg = new QString("OK\r\n");
	} catch ( ... ) {
		msg = new QString("ERROR: DB COMMUNICATION FAILED\r\n");		
	} Q_EMIT(disconnect_client(p, msg));
	m_p_mutex->lock();
	served_client = true;
	m_p_mutex->unlock();
	delete _p_text;
}

void worker_node::request_login(QString * _p_text, QTcpSocket * _p_socket)
{
	std::cout<<"request login request: \""<<_p_text->toStdString()<<"\""<<std::endl;
	/* create a tcp_connection object */
	QString client_host = _p_socket->peerName();
	tcp_connection * p = new tcp_connection(client_host, _p_socket);
	
	/* split along ':' characters */
	QStringList separated = _p_text->split(":::");
	
	if (separated.size() < 2) {
		/* if there are not enough params, disconnect. */
		QString * msg = new QString("ERROR: INVALID REQUEST\r\n");
		m_p_mutex->lock();
		served_client = true;
		m_p_mutex->unlock();
		Q_EMIT(disconnect_client(p, msg));
		return;
	}

	QString _user = separated[0];
	QString _pass = separated[1];

	QString * msg;
	/* try to authenticate */
	try {		
		if (!try_login(_user, _pass)) {
			msg = new QString("ERROR: AUTHENTICATION FAILED\r\n");
			delete _p_text;
			m_p_mutex->lock();
			served_client = true;
			m_p_mutex->unlock();
			Q_EMIT(disconnect_client(p, msg));
			return;
		} else msg = new QString("OK\r\n");
	} catch ( ... ) {
		msg = new QString("ERROR: DB COMMUNICATION FAILED\r\n");
	} Q_EMIT(disconnect_client(p, msg));
	m_p_mutex->lock();
	served_client = true;
	m_p_mutex->unlock();
	delete _p_text;
}

void worker_node::request_create_group(QString * _p_text, QTcpSocket * _p_socket)
{
	std::cout<<"request create group: \""<<_p_text->toStdString()<<"\""<<std::endl;
	/* create a tcp_connection object */
	QString client_host = _p_socket->peerName();
	tcp_connection * p = new tcp_connection(client_host, _p_socket);

	/* split along ':' characters */
	QStringList separated= _p_text->split(":::");

	if (separated.size() < 3) {
		/* invalid params => disconnect */
		QString * msg = new QString("ERROR: INVALID REQUEST\r\n");
		m_p_mutex->lock();
		served_client = true;
		m_p_mutex->unlock();
		Q_EMIT(disconnect_client(p, msg));
		return;
	}

	QString _user = separated[0];
	QString _pass = separated[1];
	QString  _grp = separated[2];

	QString * msg;

	try {
		if (!try_login(_user, _pass)) {
			std::cerr<<"Authentication Error"<<std::endl;
			msg = new QString("ERROR: AUTHENTICATION FAILED\r\n");
			Q_EMIT(disconnect_client(p, msg));
			delete _p_text;
			m_p_mutex->lock();
			served_client = true;
			m_p_mutex->unlock();
			return;
		} else if (!insert_group(_grp)) {
			msg = new QString("ERROR: EXISTING GROUP\r\n");
			m_p_mutex->lock();
			served_client = true;
			m_p_mutex->unlock();
			Q_EMIT(disconnect_client(p, msg));
			delete _p_text;
			return;
		} else msg = new QString("OK\r\n");
	} catch ( ... ) {
		msg = new QString("ERROR: DB COMMUNICATION FAILED\r\n");		
	} Q_EMIT(disconnect_client(p, msg));
	m_p_mutex->lock();
	served_client = true;
	m_p_mutex->unlock();
	delete _p_text;
}

void worker_node::request_add_to_group(QString * _p_text, QTcpSocket * _p_socket)
{
	std::cout<<"request add to group: \""<<_p_text->toStdString()<<"\""<<std::endl;
	/* create a tcp_connection object */
	QString client_host = _p_socket->peerName();
	tcp_connection * p = new tcp_connection(client_host, _p_socket);

	/* split along ':' characters */
	QStringList separated= _p_text->split(":::");

	if (separated.size() < 4) {
		/* invalid params => disconnect */
		QString * msg = new QString("ERROR: INVALID REQUEST\r\n");
		m_p_mutex->lock();
		served_client = true;
		m_p_mutex->unlock();
		Q_EMIT(disconnect_client(p, msg));
		return;
	}

	QString _user = separated[0];
	QString _pass = separated[1];
	QString  _grp = separated[2];
	QString _usr2 = separated[3];

	QString * msg;

	try {
		if (!try_login(_user, _pass)) {
			std::cerr<<"Authentication Error"<<std::endl;
			msg = new QString("ERROR: AUTHENTICATION FAILED\r\n");
			Q_EMIT(disconnect_client(p, msg));
			delete _p_text;
			m_p_mutex->lock();
			served_client = true;
			m_p_mutex->unlock();
			return;
		} else if (!join_group(_usr2, _grp)) {
			msg = new QString("ERROR: GROUP DOES NOT EXIST\r\n");
			m_p_mutex->lock();
			served_client = true;
			m_p_mutex->unlock();
			Q_EMIT(disconnect_client(p, msg));
			delete _p_text;
			return;
		} else msg = new QString("OK\r\n");
	} catch ( ... ) {
		msg = new QString("ERROR: DB COMMUNICATION FAILED\r\n");		
	} Q_EMIT(disconnect_client(p, msg));
	m_p_mutex->lock();
	served_client = true;
	m_p_mutex->unlock();
	delete _p_text;
}

void worker_node::request_delete_group(QString * _p_text, QTcpSocket * _p_socket)
{
	std::cout<<"request delete group: \""<<_p_text->toStdString()<<"\""<<std::endl;
	/* create a tcp_connection object */
	QString client_host = _p_socket->peerName();
	tcp_connection * p = new tcp_connection(client_host, _p_socket);

	/* split along ':' characters */
	QStringList separated= _p_text->split(":::");

	if (separated.size() != 3) {
		/* invalid params => disconnect */
		QString * msg = new QString("ERROR: INVALID REQUEST\r\n");
		m_p_mutex->lock();
		served_client = true;
		m_p_mutex->unlock();
		Q_EMIT(disconnect_client(p, msg));
		return;
	}

	QString _user = separated[0];
	QString _pass = separated[1];
	QString  _grp = separated[2];

	QString * msg;

	try {
		if (!try_login(_user, _pass)) {
			std::cerr<<"Authentication Error"<<std::endl;
			msg = new QString("ERROR: AUTHENTICATION FAILED\r\n");
			Q_EMIT(disconnect_client(p, msg));
			delete _p_text;
			m_p_mutex->lock();
			served_client = true;
			m_p_mutex->unlock();
			return;
		} else if (!remove_group(_grp)) {
			msg = new QString("ERROR: GROUP REMOVAL FAILED\r\n");
			m_p_mutex->lock();
			served_client = true;
			m_p_mutex->unlock();
			Q_EMIT(disconnect_client(p, msg));
			delete _p_text;
			return;
		} else msg = new QString("OK\r\n");
	} catch ( ... ) {
		msg = new QString("ERROR: DB COMMUNICATION FAILED\r\n");		
	} Q_EMIT(disconnect_client(p, msg));
	m_p_mutex->lock();
	served_client = true;
	m_p_mutex->unlock();
	delete _p_text;
}

void worker_node::request_leave_group(QString * _p_text, QTcpSocket * _p_socket)
{
	std::cout<<"request add to group: \""<<_p_text->toStdString()<<"\""<<std::endl;
	/* create a tcp_connection object */
	QString client_host = _p_socket->peerName();
	tcp_connection * p = new tcp_connection(client_host, _p_socket);

	/* split along ':' characters */
	QStringList separated= _p_text->split(":::");

	if (separated.size() < 4) {
		/* invalid params => disconnect */
		QString * msg = new QString("ERROR: INVALID REQUEST\r\n");
		m_p_mutex->lock();
		served_client = true;
		m_p_mutex->unlock();
		Q_EMIT(disconnect_client(p, msg));
		return;
	}

	QString _user = separated[0];
	QString _pass = separated[1];
	QString  _grp = separated[2];
	QString _usr2 = separated[3];

	QString * msg;

	try {
		if (!try_login(_user, _pass)) {
			std::cerr<<"Authentication Error"<<std::endl;
			msg = new QString("ERROR: AUTHENTICATION FAILED\r\n");
			Q_EMIT(disconnect_client(p, msg));
			delete _p_text;
			m_p_mutex->lock();
			served_client = true;
			m_p_mutex->unlock();
			return;
		} else if (!leave_group(_usr2, _grp)) {
			msg = new QString("ERROR: GROUP DOES NOT EXIST\r\n");
			m_p_mutex->lock();
			served_client = true;
			m_p_mutex->unlock();
			Q_EMIT(disconnect_client(p, msg));
			delete _p_text;
			return;
		} else msg = new QString("OK\r\n");
	} catch ( ... ) {
		msg = new QString("ERROR: DB COMMUNICATION FAILED\r\n");		
	} Q_EMIT(disconnect_client(p, msg));
	m_p_mutex->lock();
	served_client = true;
	m_p_mutex->unlock();
	delete _p_text;
}

void worker_node::request_update_user(QString * _p_text, QTcpSocket * _p_socket)
{
	std::cout<<"request update user: \""<<_p_text->toStdString()<<"\""<<std::endl;
	/* create a tcp_connection object */
	QString client_host = _p_socket->peerName();
	tcp_connection * p = new tcp_connection(client_host, _p_socket);

	/* split along ':' characters */
	QStringList separated= _p_text->split(":::");

	if (separated.size() < 5) {
		/* invalid params => disconnect */
		QString * msg = new QString("ERROR: INVALID REQUEST\r\n");
		m_p_mutex->lock();
		served_client = true;
		m_p_mutex->unlock();
		Q_EMIT(disconnect_client(p, msg));
		return;
	} bool has_phone = separated.size() == 6;

	QString _old_user = separated[0];
	QString _old_pass = separated[1];
	QString _new_pass = separated[2];
	QString _new_user = separated[3];
	QString _new_mail = separated[4];
	QString _new_cell = (has_phone) ? separated[5] : "";

	QString * msg;

	try {
		if (!try_login(_old_user, _old_pass)) {
			std::cerr<<"Authentication Error"<<std::endl;
			msg = new QString("ERROR: AUTHENTICATION FAILED\r\n");
			Q_EMIT(disconnect_client(p, msg));
			delete _p_text;
			m_p_mutex->lock();
			served_client = true;
			m_p_mutex->unlock();
			return;
		} else if (!update_user(_old_user, _old_pass,
								_new_pass, _new_user,
								_new_mail, _new_cell)) {
			msg = new QString("ERROR: FAILED TO UPDATE USER\r\n");
			m_p_mutex->lock();
			served_client = true;
			m_p_mutex->unlock();
			Q_EMIT(disconnect_client(p, msg));
			delete _p_text;
			return;
		} else msg = new QString("OK\r\n");
	} catch ( ... ) {
		msg = new QString("ERROR: DB COMMUNICATION FAILED\r\n");		
	} Q_EMIT(disconnect_client(p, msg));
	m_p_mutex->lock();
	served_client = true;
	m_p_mutex->unlock();
	delete _p_text;
}

void worker_node::request_personal_event(QString * _p_text, QTcpSocket * _p_socket)
{
	std::cout<<"request create personal event: \""<<_p_text->toStdString()<<"\""<<std::endl;
	/* create a tcp_connection object */
	QString client_host = _p_socket->peerName();
	tcp_connection * p = new tcp_connection(client_host, _p_socket);

	/* split along ':' characters */
	QStringList separated= _p_text->split(":::");

	if (separated.size() != 9) {
		/* invalid params => disconnect */
		QString * msg = new QString("ERROR: INVALID REQUEST\r\n");
		m_p_mutex->lock();
		served_client = true;
		m_p_mutex->unlock();
		Q_EMIT(disconnect_client(p, msg));
		return;
	}

	QString user      = separated[0];
	QString pass      = separated[1];
	QString date      = separated[2];
	QString start     = separated[3];
	QString duration  = separated[4];
	QString location  = separated[5];
	QString timezone  = separated[6];
	QString name      = separated[7];
	QString immutable = separated[8];

	QString * msg;

	try {
		if (!try_login(user, pass)) {
			std::cerr<<"Authentication Error"<<std::endl;
			msg = new QString("ERROR: AUTHENTICATION FAILED\r\n");
			Q_EMIT(disconnect_client(p, msg));
			delete _p_text;
			m_p_mutex->lock();
			served_client = true;
			m_p_mutex->unlock();
			return;
		} else if (!create_personal_event(user,     date,
										  start,    duration,
										  location, timezone,
										  name, immutable)) {
			msg = new QString("ERROR: FAILED TO CREATE EVENT\r\n");
			m_p_mutex->lock();
			served_client = true;
			m_p_mutex->unlock();
			Q_EMIT(disconnect_client(p, msg));
			delete _p_text;
			return;
		} else msg = new QString("OK\r\n");
	} catch ( ... ) {
		msg = new QString("ERROR: DB COMMUNICATION FAILED\r\n");		
	} Q_EMIT(disconnect_client(p, msg));
	m_p_mutex->lock();
	served_client = true;
	m_p_mutex->unlock();
	delete _p_text;
}

void worker_node::request_group_event(QString * _p_text, QTcpSocket * _p_socket)
{
	std::cout<<"request create group event: \""<<_p_text->toStdString()<<"\""<<std::endl;
	/* create a tcp_connection object */
	QString client_host = _p_socket->peerName();
	tcp_connection * p = new tcp_connection(client_host, _p_socket);

	/* split along ':' characters */
	QStringList separated= _p_text->split(":::");

	if (separated.size() != 9) {
		/* invalid params => disconnect */
		QString * msg = new QString("ERROR: INVALID REQUEST\r\n");
		m_p_mutex->lock();
		served_client = true;
		m_p_mutex->unlock();
		Q_EMIT(disconnect_client(p, msg));
		return;
	}

	QString user      = separated[0];
	QString pass      = separated[1];
	QString group     = separated[2];
	QString date      = separated[3];
	QString start     = separated[4];
	QString duration  = separated[5];
	QString location  = separated[6];
	QString timezone  = separated[7];
	QString name      = separated[8];
	QString immutable = "0";
	QString * msg;

	try {
		if (!try_login(user, pass)) {
			std::cerr<<"Authentication Error"<<std::endl;
			msg = new QString("ERROR: AUTHENTICATION FAILED\r\n");
			Q_EMIT(disconnect_client(p, msg));
			delete _p_text;
			m_p_mutex->lock();
			served_client = true;
			m_p_mutex->unlock();
			return;
		} else if (!create_personal_event(group,    date,
										  start,    duration,
										  location, timezone,
										  name,     immutable)) {
			msg = new QString("ERROR: FAILED TO CREATE EVENT\r\n");
			m_p_mutex->lock();
			served_client = true;
			m_p_mutex->unlock();
			Q_EMIT(disconnect_client(p, msg));
			delete _p_text;
			return;
		} else msg = new QString("OK\r\n");
	} catch ( ... ) {
		msg = new QString("ERROR: DB COMMUNICATION FAILED\r\n");		
	} Q_EMIT(disconnect_client(p, msg));
	m_p_mutex->lock();
	served_client = true;
	m_p_mutex->unlock();
	delete _p_text;
}

void worker_node::request_user_groups(QString * _p_text, QTcpSocket * _p_socket)
{
	std::cout<<"request group users: \""<<_p_text->toStdString()<<"\""<<std::endl;
	/* create a tcp_connection object */
	QString client_host = _p_socket->peerName();
	tcp_connection * p = new tcp_connection(client_host, _p_socket);

	/* split along ':' characters */
	QStringList separated= _p_text->split(":::");

	if (separated.size() != 2) {
		/* invalid params => disconnect */
		QString * msg = new QString("ERROR: INVALID REQUEST\r\n");
		m_p_mutex->lock();
		served_client = true;
		m_p_mutex->unlock();
		Q_EMIT(disconnect_client(p, msg));
		return;
	}

	QString user = separated[0];
	QString pass = separated[1];

	QString * msg;

	try {
		if (!try_login(user, pass)) {
			std::cerr<<"Authentication Error"<<std::endl;
			msg = new QString("ERROR: AUTHENTICATION FAILED\r\n");
			Q_EMIT(disconnect_client(p, msg));
			delete _p_text;
			m_p_mutex->lock();
			served_client = true;
			m_p_mutex->unlock();
			return;
		} else if (!list_groups(user, msg = new QString())) {
			msg = new QString("ERROR: FAILED TO FETCH GROUP LIST\r\n");
			m_p_mutex->lock();
			served_client = true;
			m_p_mutex->unlock();
			Q_EMIT(disconnect_client(p, msg));
			delete _p_text;
			return;
		}
	} catch ( ... ) {
		msg = new QString("ERROR: DB COMMUNICATION FAILED\r\n");		
	} Q_EMIT(disconnect_client(p, msg));
	m_p_mutex->lock();
	served_client = true;
	m_p_mutex->unlock();
	delete _p_text;
}

void worker_node::request_account_info(QString * _p_text, QTcpSocket * _p_socket)
{
	std::cout<<"request account info: \""<<_p_text->toStdString()<<"\""<<std::endl;
	/* create a tcp_connection object */
	QString client_host = _p_socket->peerName();
	tcp_connection * p = new tcp_connection(client_host, _p_socket);

	/* split along ':' characters */
	QStringList separated= _p_text->split(":::");

	if (separated.size() != 2) {
		/* invalid params => disconnect */
		QString * msg = new QString("ERROR: INVALID REQUEST\r\n");
		m_p_mutex->lock();
		served_client = true;
		m_p_mutex->unlock();
		Q_EMIT(disconnect_client(p, msg));
		return;
	}

	QString user = separated[0];
	QString pass = separated[1];

	QString * msg;

	try {
		if (!try_login(user, pass)) {
			std::cerr<<"Authentication Error"<<std::endl;
			msg = new QString("ERROR: AUTHENTICATION FAILED\r\n");
			Q_EMIT(disconnect_client(p, msg));
			delete _p_text;
			m_p_mutex->lock();
			served_client = true;
			m_p_mutex->unlock();
			return;
		} else if (!get_account_info(user, msg = new QString())) {
			msg = new QString("ERROR: FAILED TO FETCH ACCOUNT INFO\r\n");
			m_p_mutex->lock();
			served_client = true;
			m_p_mutex->unlock();
			Q_EMIT(disconnect_client(p, msg));
			delete _p_text;
			return;
		}
	} catch ( ... ) {
		msg = new QString("ERROR: DB COMMUNICATION FAILED\r\n");		
	} Q_EMIT(disconnect_client(p, msg));
	m_p_mutex->lock();
	served_client = true;
	m_p_mutex->unlock();
	delete _p_text;
}

void worker_node::request_group_users(QString * _p_text, QTcpSocket * _p_socket)
{
	std::cout<<"request group users: \""<<_p_text->toStdString()<<"\""<<std::endl;
	/* create a tcp_connection object */
	QString client_host = _p_socket->peerName();
	tcp_connection * p = new tcp_connection(client_host, _p_socket);

	/* split along ':' characters */
	QStringList separated= _p_text->split(":::");

	if (separated.size() != 3) {
		/* invalid params => disconnect */
		QString * msg = new QString("ERROR: INVALID REQUEST\r\n");
		m_p_mutex->lock();
		served_client = true;
		m_p_mutex->unlock();
		Q_EMIT(disconnect_client(p, msg));
		return;
	}

	QString user  = separated[0];
	QString pass  = separated[1];
	QString group = separated[2];

	QString * msg;

	try {
		if (!try_login(user, pass)) {
			std::cerr<<"Authentication Error"<<std::endl;
			msg = new QString("ERROR: AUTHENTICATION FAILED\r\n");
			Q_EMIT(disconnect_client(p, msg));
			delete _p_text;
			m_p_mutex->lock();
			served_client = true;
			m_p_mutex->unlock();
			return;
		} else if (!list_group_users(group, msg = new QString())) {
			msg = new QString("ERROR: FAILED TO FETCH GROUP USERS\r\n");
			m_p_mutex->lock();
			served_client = true;
			m_p_mutex->unlock();
			Q_EMIT(disconnect_client(p, msg));
			delete _p_text;
			return;
		}
	} catch ( ... ) {
		msg = new QString("ERROR: DB COMMUNICATION FAILED\r\n");		
	} Q_EMIT(disconnect_client(p, msg));
	m_p_mutex->lock();
	served_client = true;
	m_p_mutex->unlock();
	delete _p_text;
}

void worker_node::request_user_events(QString * _p_text, QTcpSocket * _p_socket)
{
	std::cout<<"request user events: \""<<_p_text->toStdString()<<"\""<<std::endl;
	/* create a tcp_connection object */
	QString client_host = _p_socket->peerName();
	tcp_connection * p = new tcp_connection(client_host, _p_socket);

	/* split along ':' characters */
	QStringList separated= _p_text->split(":::");

	if (separated.size() != 4) {
		/* invalid params => disconnect */
		QString * msg = new QString("ERROR: INVALID REQUEST\r\n");
		m_p_mutex->lock();
		served_client = true;
		m_p_mutex->unlock();
		Q_EMIT(disconnect_client(p, msg));
		return;
	}

	QString user  = separated[0];
	QString pass  = separated[1];
	QString start_day = separated[2];
	QString stop_day = separated[3];

	QString * msg;

	try {
		if (!try_login(user, pass)) {
			std::cerr<<"Authentication Error"<<std::endl;
			msg = new QString("ERROR: AUTHENTICATION FAILED\r\n");
			Q_EMIT(disconnect_client(p, msg));
			delete _p_text;
			m_p_mutex->lock();
			served_client = true;
			m_p_mutex->unlock();
			return;
		} else if (!list_user_events(user, start_day, stop_day, msg = new QString())) {
			msg = new QString("ERROR: FAILED TO FETCH USER EVENTS\r\n");
			m_p_mutex->lock();
			served_client = true;
			m_p_mutex->unlock();
			Q_EMIT(disconnect_client(p, msg));
			delete _p_text;
			return;
		}
	} catch ( ... ) {
		msg = new QString("ERROR: DB COMMUNICATION FAILED\r\n");		
	} Q_EMIT(disconnect_client(p, msg));
	m_p_mutex->lock();
	served_client = true;
	m_p_mutex->unlock();
	delete _p_text;
}

void worker_node::request_group_events(QString * _p_text, QTcpSocket * _p_socket)
{
	std::cout<<"request group events: \""<<_p_text->toStdString()<<"\""<<std::endl;
	/* create a tcp_connection object */
	QString client_host = _p_socket->peerName();
	tcp_connection * p = new tcp_connection(client_host, _p_socket);

	/* split along ':' characters */
	QStringList separated= _p_text->split(":::");

	if (separated.size() != 5) {
		/* invalid params => disconnect */
		QString * msg = new QString("ERROR: INVALID REQUEST\r\n");
		m_p_mutex->lock();
		served_client = true;
		m_p_mutex->unlock();
		Q_EMIT(disconnect_client(p, msg));
		return;
	}

	QString user  = separated[0];
	QString pass  = separated[1];
	QString start_day = separated[2];
	QString stop_day = separated[3];
	QString group = separated[4];

	QString * msg;

	try {
		if (!try_login(user, pass)) {
			std::cerr<<"Authentication Error"<<std::endl;
			msg = new QString("ERROR: AUTHENTICATION FAILED\r\n");
			Q_EMIT(disconnect_client(p, msg));
			delete _p_text;
			m_p_mutex->lock();
			served_client = true;
			m_p_mutex->unlock();
			return;
		} else if (!user_in_group(user, group)) {
			msg = new QString("ERROR: USER ");
			*msg += "\"" + user + "\" IS NOT IN GROUP \"" + group + "\"\r\n";
			m_p_mutex->lock();
			served_client = true;
			m_p_mutex->unlock();
			Q_EMIT(disconnect_client(p, msg));
			delete _p_text;
			return;
		} else if (!list_user_events(group, start_day, stop_day, msg = new QString())) {
			msg = new QString("ERROR: FAILED TO FETCH USER EVENTS\r\n");
			m_p_mutex->lock();
			served_client = true;
			m_p_mutex->unlock();
			Q_EMIT(disconnect_client(p, msg));
			delete _p_text;
			return;
		}
	} catch ( ... ) {
		msg = new QString("ERROR: DB COMMUNICATION FAILED\r\n");		
	} Q_EMIT(disconnect_client(p, msg));
	m_p_mutex->lock();
	served_client = true;
	m_p_mutex->unlock();
	delete _p_text;
}

void worker_node::request_personal_month_events(QString * _p_text, QTcpSocket * _p_socket)
{
	std::cout<<"request user month events: \""<<_p_text->toStdString()
			 <<"\""<<std::endl;
	/* create a tcp_connection object */
	QString client_host = _p_socket->peerName();
	tcp_connection * p = new tcp_connection(client_host, _p_socket);

	/* split along ':' characters */
	QStringList separated= _p_text->split(":::");

	if (separated.size() != 4) {
		/* invalid params => disconnect */
		QString * msg = new QString("ERROR: INVALID REQUEST\r\n");
		m_p_mutex->lock();
		served_client = true;
		m_p_mutex->unlock();
		Q_EMIT(disconnect_client(p, msg));
		return;
	}

	QString user  = separated[0];
	QString pass  = separated[1];
	quint8 month  = separated[2].toInt();
	quint16 year  = separated[3].toInt();

	QString * msg;

	try {
		if (!try_login(user, pass)) {
			std::cerr<<"Authentication Error"<<std::endl;
			msg = new QString("ERROR: AUTHENTICATION FAILED\r\n");
			Q_EMIT(disconnect_client(p, msg));
			delete _p_text;
			m_p_mutex->lock();
			served_client = true;
			m_p_mutex->unlock();
			return;
		} else if (!list_user_month_events(user, month, year, msg = new QString())) {
			msg = new QString("ERROR: FAILED TO FETCH USER EVENTS\r\n");
			m_p_mutex->lock();
			served_client = true;
			m_p_mutex->unlock();
			Q_EMIT(disconnect_client(p, msg));
			delete _p_text;
			return;
		}
	} catch ( ... ) {
		msg = new QString("ERROR: DB COMMUNICATION FAILED\r\n");		
	} Q_EMIT(disconnect_client(p, msg));
	m_p_mutex->lock();
	served_client = true;
	m_p_mutex->unlock();
	delete _p_text;
}

void worker_node::request_group_month_events(QString * _p_text, QTcpSocket * _p_socket)
{
	std::cout<<"request group month events: \""<<_p_text->toStdString()
			 <<"\""<<std::endl;
	/* create a tcp_connection object */
	QString client_host = _p_socket->peerName();
	tcp_connection * p = new tcp_connection(client_host, _p_socket);

	/* split along ':' characters */
	QStringList separated= _p_text->split(":::");

	if (separated.size() != 5) {
		/* invalid params => disconnect */
		QString * msg = new QString("ERROR: INVALID REQUEST\r\n");
		m_p_mutex->lock();
		served_client = true;
		m_p_mutex->unlock();
		Q_EMIT(disconnect_client(p, msg));
		return;
	}

	QString user  = separated[0];
	QString pass  = separated[1];
	quint8 month  = separated[2].toInt();
	quint16 year  = separated[3].toInt();
	QString group = separated[4];

	QString * msg;

	try {
		if (!try_login(user, pass)) {
			std::cerr<<"Authentication Error"<<std::endl;
			msg = new QString("ERROR: AUTHENTICATION FAILED\r\n");
			Q_EMIT(disconnect_client(p, msg));
			delete _p_text;
			m_p_mutex->lock();
			served_client = true;
			m_p_mutex->unlock();
			return;
		} else if (!group_exists(group)) {
			msg = new QString("ERROR: GROUP ");
			*msg += "\"" + group + "\" DOES NOT EXIST\r\n";
			m_p_mutex->lock();
			served_client = true;
			m_p_mutex->unlock();
			Q_EMIT(disconnect_client(p, msg));
			delete _p_text;
			return;
		} else if (!user_in_group(user, group)) {
			msg = new QString("ERROR: USER ");
			*msg += "\"" + user + "\" IS NOT IN GROUP \"" + group + "\"\r\n";
			m_p_mutex->lock();
			served_client = true;
			m_p_mutex->unlock();
			Q_EMIT(disconnect_client(p, msg));
			delete _p_text;
			return;
		} else if (!list_user_month_events(group, month, year, msg = new QString())) {
			msg = new QString("ERROR: FAILED TO FETCH GROUP EVENTS\r\n");
			m_p_mutex->lock();
			served_client = true;
			m_p_mutex->unlock();
			Q_EMIT(disconnect_client(p, msg));
			delete _p_text;
			return;
		}
	} catch ( ... ) {
		msg = new QString("ERROR: DB COMMUNICATION FAILED\r\n");		
	} Q_EMIT(disconnect_client(p, msg));
	m_p_mutex->lock();
	served_client = true;
	m_p_mutex->unlock();
	delete _p_text;
}

void worker_node::request_suggest_user_times(QString * _p_text, QTcpSocket * _p_socket)
{
	std::cout<<"request suggest user times: \""<<_p_text->toStdString()<<"\""<<std::endl;
	/* create a tcp_connection object */
	QString client_host = _p_socket->peerName();
	tcp_connection * p = new tcp_connection(client_host, _p_socket);

	/* split along ':' characters */
	QStringList separated= _p_text->split(":::");

	if (separated.size() != 5) {
		/* invalid params => disconnect */
		QString * msg = new QString("ERROR: INVALID REQUEST\r\n");
		m_p_mutex->lock();
		served_client = true;
		m_p_mutex->unlock();
		Q_EMIT(disconnect_client(p, msg));
		return;
	}

	QString user      = separated[0];
	QString pass      = separated[1];
	QString stop_day  = separated[2];
	QString stop_time = separated[3];
	QString duration  = separated[4];	

	QString * msg;

	try {
		if (!try_login(user, pass)) {
			std::cerr<<"Authentication Error"<<std::endl;
			msg = new QString("ERROR: AUTHENTICATION FAILED\r\n");
			Q_EMIT(disconnect_client(p, msg));
			delete _p_text;
			m_p_mutex->lock();
			served_client = true;
			m_p_mutex->unlock();
			return;
		} else if (!suggest_user_events(user, stop_day, stop_time,
										duration, msg = new QString())) {
			msg = new QString("ERROR: FAILED TO ESTIMATE EVENTS\r\n");
			m_p_mutex->lock();
			served_client = true;
			m_p_mutex->unlock();
			Q_EMIT(disconnect_client(p, msg));
			delete _p_text;
			return;
		}
	} catch ( ... ) {
		msg = new QString("ERROR: DB COMMUNICATION FAILED\r\n");		
	} Q_EMIT(disconnect_client(p, msg));
	m_p_mutex->lock();
	served_client = true;
	m_p_mutex->unlock();
	delete _p_text;
}

void worker_node::request_suggest_group_times(QString * _p_text, QTcpSocket * _p_socket)
{
	std::cout<<"request suggest group times: \""<<_p_text->toStdString()<<"\""<<std::endl;
	/* create a tcp_connection object */
	QString client_host = _p_socket->peerName();
	tcp_connection * p = new tcp_connection(client_host, _p_socket);

	/* split along ':' characters */
	QStringList separated= _p_text->split(":::");

	if (separated.size() != 6) {
		/* invalid params => disconnect */
		QString * msg = new QString("ERROR: INVALID REQUEST\r\n");
		m_p_mutex->lock();
		served_client = true;
		m_p_mutex->unlock();
		Q_EMIT(disconnect_client(p, msg));
		return;
	}

	QString user      = separated[0];
	QString pass      = separated[1];
	QString group     = separated[2];
	QString stop_day  = separated[3];
	QString stop_time = separated[4];
	QString duration  = separated[5];	

	QString * msg;

	try {
		if (!try_login(user, pass)) {
			std::cerr<<"Authentication Error"<<std::endl;
			msg = new QString("ERROR: AUTHENTICATION FAILED\r\n");
			Q_EMIT(disconnect_client(p, msg));
			delete _p_text;
			m_p_mutex->lock();
			served_client = true;
			m_p_mutex->unlock();
			return;
		} else if (!group_exists(group)) {
			msg = new QString("ERROR: GROUP ");
			*msg += "\"" + group + "\" DOES NOT EXIST\r\n";
			m_p_mutex->lock();
			served_client = true;
			m_p_mutex->unlock();
			Q_EMIT(disconnect_client(p, msg));
			delete _p_text;
			return;
		} else if (!user_in_group(user, group)) {
			msg = new QString("ERROR: USER ");
			*msg += "\"" + user + "\" IS NOT IN GROUP \"" + group + "\"\r\n";
			m_p_mutex->lock();
			served_client = true;
			m_p_mutex->unlock();
			Q_EMIT(disconnect_client(p, msg));
			delete _p_text;
			return;
		} else if (!suggest_group_events(group, stop_day, stop_time,
										 duration, msg = new QString())) {
			msg = new QString("ERROR: FAILED TO ESTIMATE EVENTS\r\n");
			m_p_mutex->lock();
			served_client = true;
			m_p_mutex->unlock();
			Q_EMIT(disconnect_client(p, msg));
			delete _p_text;
			return;
		}
	} catch (std::invalid_argument e) {
		msg = new QString(e.what());
		*msg = QString("ERROR: Exception was thrown: \"") + *msg + "\"\r\n";
	} catch ( ... ) {
		msg = new QString("ERROR: DB COMMUNICATION FAILED\r\n");		
	} Q_EMIT(disconnect_client(p, msg));
	m_p_mutex->lock();
	served_client = true;
	m_p_mutex->unlock();
	delete _p_text;
}
