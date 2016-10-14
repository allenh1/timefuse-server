#ifndef IRC_COMM_H
#define IRC_COMM_H

#include <QtCore>
#include <QObject>
#include <QTcpSocket>

class IRCComm : public QObject
{
public:
   IRCComm(QString _host = "localhost")
	  : m_host(_host)
  { /* tcp_comm */ }

   QString * getHost()  { return &m_host; }

   static const unsigned int MAX_RESPONSE = 100 * 1024;
   /* 0.5 second timieout */
   static const unsigned int TIMEOUT = 500;

   void setHost(QString host)
	  {
		 QMutex mutex;
		 mutex.lock();
		 m_host = host;
		 mutex.unlock();
	  }

   bool reconnect(int port) {
	  QMutex mutex;
	  mutex.lock();
	  QTcpSocket * pSocket = new QTcpSocket();
	  pSocket->connectToHost(m_host, port, QIODevice::ReadWrite);
	  bool toReturn = pSocket->waitForConnected(TIMEOUT);
	  pSocket->abort();
	  delete pSocket;
	  mutex.unlock();
	  return toReturn;
   }

   int sendCommand(char *  host, int port, char * command, char * response)
	  {
		 QMutex * pMutex = new QMutex();
		 pMutex->lock();
		 QTcpSocket * pSocket = new QTcpSocket();

		 QString copy = m_host;
		 pSocket->connectToHost(copy, port, QIODevice::ReadWrite);
		 pSocket->waitForConnected(TIMEOUT);
		 if (pSocket->state() == QAbstractSocket::UnconnectedState) {
			pMutex->unlock();
			delete pMutex;
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
		 pMutex->unlock();
		 delete pMutex;
		 pSocket->abort();
		 delete pSocket;
		 return 1;
	  }
private:
   QString m_host;
};//end class
#endif
