#include "tcp_thread.hpp"
/* @todo #include "User.h" */

tcp_thread::tcp_thread(
   const QString & _hostname,                /*  hostname for TCP thread construction  */
   const quint16 & _port,                    /* port on which we construct this server */
   QObject * parent                          /*         parent of this QObject         */
   )
   : QObject(parent)
{
   m_hostname = _hostname;
   m_port = _port;
   m_pServer = new QTcpServer(this);
   m_pTcpMessages = new QQueue<tcp_connection>();
}

bool tcp_thread::init()
{
   /* forward accepted connections to our accept connection */
   connect(m_pServer, &QTcpServer::newConnection, this, &tcp_thread::acceptConnection);

   /* listen for any host on our port */
   if (m_pServer->listen(QHostAddress::Any, m_port)) {
	  QString ipAddress = QHostAddress(QHostAddress::LocalHost).toString();
	  std::cout<< tr("The server is running on\n\nIP: %1\nPort: %2\n")
		 .arg(ipAddress).arg(m_pServer->serverPort()).toStdString() << std::endl;
   } else return false;
   return true;
}

bool tcp_thread::writeData(QByteArray data, QString match)
{
   QDataStream out(&data, QIODevice::WriteOnly);
   out.setVersion(QDataStream::Qt_5_7);
   TcpMessage msg;
   msg.line = match;

   // for (int x = 0; x < m_pTcpMessages->size(); ++x) {
   // 	  if (m_pTcpMessages->at(x).line == msg.line) {
   // 		 index = x;
   // 		 break;
   // 	  }
   // }

   // if (index == -1) return false;
   // QTcpSocket * sender = m_pTcpMessages->at(index).pSocket;
   // pMutex->unlock();

   // delete pMutex;

   // connect(sender, &QAbstractSocket::disconnected, sender, &QObject::deleteLater);

   // sender->write(data);
   // sender->disconnectFromHost();
	
   return true;
}

void tcp_thread::disconnected()
{
   QTcpSocket * quitter = qobject_cast<QTcpSocket *>(sender());

   std::cout<< "Client "<< QHostAddress(quitter->peerAddress().toIPv4Address()).toString().toStdString();
   std::cout<<":" << quitter->peerPort();
   std::cout<< " has left the server." << std::endl;

   //! Dequeue them, because they are quitters and meanie heads.
   QMutex * pMutex = new QMutex();
   pMutex->lock();
   for (int x = 0; x < m_pTcpMessages->size(); ++x) {
	  // if (m_pTcpMessages->at(x).pSocket == quitter) {
	  // 	 m_pTcpMessages->removeAt(x);
	  // 	 break;
	  // }
   }
   pMutex->unlock();
   delete pMutex;
}

void tcp_thread::acceptConnection()
{
   QTcpSocket * client = m_pServer->nextPendingConnection();

   if (client) {
	  connect(client, &QAbstractSocket::disconnected, client, &QObject::deleteLater);
	  connect(client, &QAbstractSocket::disconnected, this, &tcp_thread::disconnected);
	  connect(client, &QIODevice::readyRead, this, &tcp_thread::readFromClient);
   }
}

void tcp_thread::echoReceived(QString msg)
{
   std::cout<<"I read \""<<msg.toStdString()<<"\" from the client!"<<std::endl;
}

void tcp_thread::readFromClient()
{
   //! Points at the thing that called this member function,
   //! as well as casts it it a QTcpSocket.
   QTcpSocket * pClientSocket = qobject_cast<QTcpSocket *>(sender());
   
   QString text;/* to store the message */

   QByteArray bae = pClientSocket->readLine();
   QString temp = QString(bae);
   QString hostname = pClientSocket->peerName();
   text += temp.replace("\r\n", "");
   if (text == "REQUEST_CLIENT") {
	  /* this is a worker */
	  worker_connection * w = new worker_connection(hostname, pClientSocket);
	  Q_EMIT worker_connected(w);
	  std::cerr<<"Emmitted worker connect"<<std::endl;
   } else if (text == "REQUEST_WORKER") {
	  /* this is a client */
	  client_connection * c = new client_connection(hostname, pClientSocket);
	  Q_EMIT client_connected(c);
	  std::cerr<<"Emmitted client connect"<<std::endl;
   }
    // m_pTcpMessages->enqueue(toEnqueue);

    /**
	* @todo save the client somehow, probably through passing and delays.
	* This will allow us to respond to them sometime. :)
	*/
}

void tcp_thread::sendMessage(QString msg, QString request)
{
   if (!writeData(msg.toUtf8(), request)) {
	  std::cerr<<"Unable to write message \""<<msg.toStdString()<<"\" to client."<<std::endl;
   }
}
