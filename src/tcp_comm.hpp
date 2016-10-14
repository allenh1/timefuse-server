#ifndef __TCP_COMM_HPP__
#define __TCP_COMM_HPP__
#include <QtCore>
#include <QObject>
#include <QTcpSocket>

class tcp_comm : public QObject
{
public:
   explicit tcp_comm(QString _host = "localhost")
	  : m_host(_host)
	  { m_p_mutex = new QMutex(); }
   virtual ~tcp_comm() { delete m_p_mutex; }
   
   QString * getHost()  { return &m_host; }

   static const unsigned int MAX_RESPONSE = 100 * 1024;
   /* 0.5 second timieout */
   static const unsigned int TIMEOUT = 500;

   void setHost(QString host) {
	  m_p_mutex->lock();
	  m_host = host;
	  m_p_mutex->unlock();
   }

   bool reconnect(int port) {
	  m_p_mutex->lock();
	  QTcpSocket * pSocket = new QTcpSocket();
	  pSocket->connectToHost(m_host, port, QIODevice::ReadWrite);
	  bool toReturn = pSocket->waitForConnected(TIMEOUT);
	  pSocket->abort();
	  delete pSocket;
	  m_p_mutex->unlock();
	  return toReturn;
   }

   int sendCommand(char *  host, int port, char * command, char * response) {
	  m_p_mutex->lock();
	  QTcpSocket * pSocket = new QTcpSocket();

	  QString copy = m_host;
	  pSocket->connectToHost(copy, port, QIODevice::ReadWrite);
	  pSocket->waitForConnected(TIMEOUT);
	  if (pSocket->state() == QAbstractSocket::UnconnectedState) {
		 m_p_mutex->unlock();
		 pSocket->abort();
		 delete pSocket;
		 return 0;
	  }

	  pSocket->waitForReadyRead(-1);
	  QString iHeard;

	  while (pSocket->canReadLine()) {
		 iHeard += QString(pSocket->readLine());
	  }

	  QString toSend(command);
	  toSend += "\r\n";
	  pSocket->write(toSend.toUtf8());
	  pSocket->waitForBytesWritten(-1);
	  pSocket->waitForReadyRead(-1);
	  while (pSocket->canReadLine()) {
		 iHeard += QString(pSocket->readLine());
	  }
	  memcpy(response, iHeard.toStdString().c_str(), iHeard.size());
	  m_p_mutex->unlock();
	  pSocket->abort();
	  delete pSocket;
	  return 1;
   }
private:
   QString m_host;
   QMutex * m_p_mutex;
};//end class
#endif
