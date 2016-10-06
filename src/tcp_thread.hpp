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

   enum states { hello, wait_request, reject };
   
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
			if (!m_pTcpMessages->at(x).read)
			   ++size;
		 }
	  }
	  pMutex->unlock();
	  delete pMutex;
	  return size;
   }

   QString getLastMessage() {
	  QMutex * pMutex = new QMutex();
	  pMutex->lock();
	  QString line;
	  for (int x = m_pTcpMessages->size() - 1; x >= 0; --x) {
		 if (!m_pTcpMessages->at(x).read) {
			line = m_pTcpMessages->at(x).line;
			/**
			 * @todo Apparently we actually have to replace this.
			 */
			TcpMessage temp = m_pTcpMessages->takeAt(x);
			temp.read = true;
			m_pTcpMessages->insert(x, temp);
			break;
		 }
	  }
	  pMutex->unlock();
	  delete pMutex;
	  return line;
   }

   const QTcpServer * getServer() { return m_pServer; }
private:
   QTcpServer * m_pServer;

   volatile bool m_continue = true;

   QString m_hostname;
   quint16 m_port;
   quint16 m_blockSize;

   QQueue<TcpMessage> * m_pTcpMessages;
};
#endif
