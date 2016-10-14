#ifndef __TCP_THREAD_HPP__
#define __TCP_THREAD_HPP__

#include <iostream>

#include <QDataStream>
#include <QtNetwork>
#include <QtCore>
#include <QDebug>

#include "worker_connection.hpp"
#include "client_connection.hpp"
#include "master_node.hpp"

class master_node;

class tcp_thread : public QObject
{
   Q_OBJECT
public:
   explicit tcp_thread(const QString & _hostname, const quint16 & _port, QObject * parent = NULL);
   ~tcp_thread() { /** @todo This function is important, I suppose... **/ }

   bool init();
   bool writeData(QByteArray data, tcp_connection * receiver);
   
   Q_SLOT void disconnected();
   Q_SLOT void readFromClient();
   Q_SLOT void sendMessage(QString, tcp_connection * request);
   Q_SLOT void acceptConnection();
   Q_SLOT void stop(){ m_continue = false; }
   Q_SLOT void send_pair_info(tcp_connection * request);
   
   Q_SIGNAL void readIt(QTcpSocket*);
   Q_SIGNAL void receivedMessage();

   Q_SIGNAL void worker_connected(worker_connection * _worker);
   Q_SIGNAL void client_connected(client_connection * _client);
   
   Q_SLOT void echoReceived(QString);

   int queueDepth() {
	  QMutex * pMutex = new QMutex();
	  pMutex->lock();
	  int queueSize = m_pTcpMessages->size();
	  int size = 0;
	  if (!queueSize) {
		 pMutex->unlock();
		 delete pMutex;
		 return queueSize;
	  }

	  else {
		 for (int x = 0; x < queueSize; ++x) {
			++size;
		 }
	  }
	  pMutex->unlock();
	  delete pMutex;
	  return size;
   }

   void set_master(master_node * _p_master_node) { m_p_master_node = _p_master_node; }

   const master_node * get_master() { return m_p_master_node; }
   const QTcpServer * getServer() { return m_pServer; }
private:
   QTcpServer * m_pServer;

   volatile bool m_continue = true;

   QString m_hostname;
   quint16 m_port;
   quint16 m_blockSize;

   master_node * m_p_master_node;
   
   QQueue<tcp_connection> * m_pTcpMessages;
};
#endif
