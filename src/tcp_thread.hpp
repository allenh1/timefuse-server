#ifndef __TCP_THREAD_HPP__
#define __TCP_THREAD_HPP__

#include <iostream>

#include <QDataStream>
#include <QtNetwork>
#include <QtCore>
#include <QDebug>

#include "worker_connection.hpp"
#include "client_connection.hpp"

struct TcpMessage {
   QString line;
   QTcpSocket * pSocket;

   bool read = false;

   bool operator == (TcpMessage m1) {
	  return line == m1.line;
   }
};

class tcp_thread : public QObject
{
   Q_OBJECT
public:
   explicit tcp_thread(const QString & _hostname, const quint16 & _port, QObject * parent = NULL);
   ~tcp_thread() { /** @todo This function is important, I suppose... **/ }

   bool init();
   bool writeData(QByteArray data, QString match);
   
   Q_SLOT void disconnected();
   Q_SLOT void readFromClient();
   Q_SLOT void sendMessage(QString, QString);
   Q_SLOT void acceptConnection();
   Q_SLOT void stop(){ m_continue = false; }

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

   const QTcpServer * getServer() { return m_pServer; }
private:
   QTcpServer * m_pServer;

   volatile bool m_continue = true;

   QString m_hostname;
   quint16 m_port;
   quint16 m_blockSize;

   QQueue<tcp_connection> * m_pTcpMessages;
};
#endif
