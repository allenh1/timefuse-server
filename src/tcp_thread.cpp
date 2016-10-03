#include "TcpThread.h"
#include "User.h"

TcpThread::TcpThread(
   const QString & _hostname,                /*  hostname for TCP thread construction  */
   const quint16 & _port,                    /* port on which we construct this server */
   QObject * parent                          /*         parent of this QObject         */
   )
   : QObject(parent)
{
   m_hostname = _hostname;
   m_port = _port;
   m_pServer = new QTcpServer(this);
   m_pTcpMessages = new QQueue<TcpMessage>();
}

bool TcpThread::init()
{
   connect(m_pServer, &QTcpServer::newConnection, this, &TcpThread::acceptConnection);
   if (m_pServer->listen(QHostAddress::Any, m_port)) {
	  QString ipAddress = QHostAddress(QHostAddress::LocalHost).toString();
	  std::cout<< tr("The server is running on\n\nIP: %1\nPort: %2\n")
		 .arg(ipAddress).arg(m_pServer->serverPort()).toStdString() << std::endl;
   } else return false;
   return true;
}

bool TcpThread::writeData(QByteArray data, QString match)
{
   QDataStream out(&data, QIODevice::WriteOnly);
   out.setVersion(QDataStream::Qt_5_5);
   TcpMessage msg;
   msg.line = match;

   QMutex * pMutex = new QMutex();
   int index = -1;
   pMutex->lock();

   for (int x = 0; x < m_pTcpMessages->size(); ++x) {
	  if (m_pTcpMessages->at(x).line == msg.line) {
		 index = x;
		 break;
	  }
   }
   
   if (index == -1) return false;
   QTcpSocket * sender = m_pTcpMessages->at(index).pSocket;
   pMutex->unlock();

   delete pMutex;

   connect(sender, &QAbstractSocket::disconnected, sender, &QObject::deleteLater);

   sender->write(data);
   sender->disconnectFromHost();
	
   return true;
}

void TcpThread::disconnected()
{
   QTcpSocket * quitter = qobject_cast<QTcpSocket *>(sender());

   std::cout<< "Client "<< QHostAddress(quitter->peerAddress().toIPv4Address()).toString().toStdString();
   std::cout<<":" << quitter->peerPort();
   std::cout<< " has left the server." << std::endl;

   //! Dequeue them, because they are quitters and meanie heads.
   QMutex * pMutex = new QMutex();
   pMutex->lock();
   for (int x = 0; x < m_pTcpMessages->size(); ++x) {
	  if (m_pTcpMessages->at(x).pSocket == quitter) {
		 m_pTcpMessages->removeAt(x);
		 break;
	  }
   }
   pMutex->unlock();
   delete pMutex;
}

void TcpThread::acceptConnection()
{
   QTcpSocket * client = m_pServer->nextPendingConnection();
   client->write("Hello there!\r\n");
   client->flush();

   if (client) {
	  connect(client, &QAbstractSocket::disconnected, client, &QObject::deleteLater);
	  connect(client, &QAbstractSocket::disconnected, this, &TcpThread::disconnected);
	  connect(client, &QIODevice::readyRead, this, &TcpThread::readFromClient);
   }
}

void TcpThread::echoReceived(QString msg)
{
   std::cout<<"I read \""<<msg.toStdString()<<"\" from the client!"<<std::endl;
}

void TcpThread::readFromClient()
{
   //! Points at the thing that called this member function,
   //! as well as casts it it a QTcpSocket.
   QTcpSocket * pClientSocket = qobject_cast<QTcpSocket *>(sender());

   QString text; TcpMessage toEnqueue;
   for (;pClientSocket->canReadLine();) {
	  QByteArray bae = pClientSocket->readLine();
	  QString temp = QString(bae);
	  text += temp.replace("\r\n", "");
   }
   if (text == tr(""))
	  return;

   toEnqueue.line = text;
   toEnqueue.pSocket = pClientSocket;
   m_pTcpMessages->enqueue(toEnqueue);
   Q_EMIT receivedMessage();
   /**
	* @todo save the client somehow, probably through passing and delays.
	* This will allow us to respond to them sometime. :)
	*/
}

void TcpThread::sendMessage(QString msg, QString request)
{
   if (!writeData(msg.toUtf8(), request)) {
	  std::cerr<<"Unable to write message \""<<msg.toStdString()<<"\" to client."<<std::endl;
   }
}
