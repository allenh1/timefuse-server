#ifndef __WORKER_NODE_HPP__
#define __WORKER_NODE_HPP__
#pragma once

/* Qt Includes */
#include <stdexcept>
#include <QtNetwork>
#include <QSqlQuery>
#include <QtCore>

/* File Includes */
#include "tcp_thread.hpp"
#include "thread_init_exception.hpp"
#include "user.hpp"

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

   Q_SIGNAL void established_client_connection();
   Q_SIGNAL void finished_client_job();
   
   Q_SLOT void run();
   Q_SLOT void stop() { m_continue = false; }
   Q_SLOT void start_thread() { m_p_thread->start(); }
private:
   volatile bool m_continue = true;
   
   QString m_hostname;
   quint16 m_port;

   tcp_thread * m_p_tcp_thread;
   QThread * m_p_thread;

   quint16 sleep_time = 400;

   Q_SLOT QSqlDatabase setup_db();
   Q_SLOT bool insert_query(const User & user);
};
#endif
