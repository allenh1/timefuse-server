#ifndef __MASTER_HPP__
#define __MASTER_HPP__
/* Qt Includes */
#include <QThread>
#include <QtCore>

/* File Includes */
#include "tcp_thread.hpp"
#include "slave.hpp"

/**
 * TimeFuse Master class.
 *
 * This is the thread on which the
 * master node decides how to/if it
 * can conect a user to a worker node.
 *
 * @author Hunter Allen <allen286@purdue.edu>
 */
class master_node : public QObject
{
   Q_OBJECT
public:
   explicit master_node(const QString & _hostname,     /* master node's hostname */
						const quint16 & _port,         /* master node's port num */
						QObject * _p_parent = NULL       /* master node's parent   */
	  );
   
   virtual ~master_node();

   bool init();

   /* Thread's Run Function */
   Q_SLOT void run();

   /* Callback functions for tcp connections */
   Q_SLOT void handle_client_connect(client_connection * _client);
   Q_SLOT void handle_worker_connect(worker_connection * _worker);

   Q_SLOT void stop() { m_continue = false; }
private:
   volatile bool m_continue = true;

   QString m_hostname;
   quint16 m_port;

   tcp_thread * m_p_tcp_thread;
   QThread * m_p_thread;

   QQueue<worker_connection *> m_worker_connections;
   QQueue<client_connection *> m_client_connections;

   QSemaphore * m_p_client_sema; QMutex * m_p_client_mutex;
   QSemaphore * m_p_worker_sema; QMutex * m_p_worker_mutex;
};
#endif

