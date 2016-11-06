#ifndef __WORKER_NODE_HPP__
#define __WORKER_NODE_HPP__
#pragma once

/* Qt Includes */
#include <stdexcept>
#include <QtNetwork>
#include <QSqlRecord>
#include <QSqlQuery>
#include <QtCore>

/* File Includes */
#include "user.hpp"
#include "tcp_comm.hpp"
#include "tcp_thread.hpp"
#include "thread_init_exception.hpp"
#include "worker_connection_state.hpp"

class tcp_thread;

class worker_node : public QObject
{
	Q_OBJECT
   
public:
	explicit worker_node(const QString & _hostname,
						 const quint16 & _port,
						 QObject * _p_parent = NULL);
	virtual ~worker_node();

	bool init();

	bool try_login(const QString & _user,
				   const QString & _password);
	bool try_create(const QString & _user,
					const QString & _password,
					const QString & _email);
   
	Q_SIGNAL void established_client_connection();
	Q_SIGNAL void finished_client_job();
   
	Q_SLOT void run();
	Q_SLOT void stop() { m_continue = false; }
	Q_SLOT void start_thread() { m_p_thread->start(); }

	Q_SLOT QSqlDatabase setup_db();
	Q_SLOT bool insert_user(user & u);
	Q_SLOT bool select_user(user & u);
	Q_SLOT bool select_schedule_id(user & u);

	bool username_exists(const QString & _user);
	bool cleanup_db_insert();
   
	void set_master_hostname(const QString & _master_host) {
		m_master_host = _master_host;
	}

	void set_master_port(const quint16 & _master_port) {
		m_master_port = _master_port;
	}

	Q_SIGNAL void disconnect_client(tcp_connection * client,
									QString * _p_msg);
	Q_SLOT void request_create_account(QString * _p_text,
									   QTcpSocket * _p_socket);
	Q_SLOT void request_login(QString * _p_text,
							  QTcpSocket * _p_socket);
private:
	volatile bool m_continue = true;
   
	QString m_host;
	quint16 m_port;

	QString m_master_host = "localhost";
	quint16 m_master_port = 3224;
   
	tcp_thread * m_p_tcp_thread;
	QThread * m_p_thread;

	connection_state state; /* state enum for the state machine */
   
	quint16 sleep_time = 400;
	QSqlDatabase m_db;
	volatile bool served_client = false;
	QMutex * m_p_mutex;
};
#endif
