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
   m_pTcpMessages = new QQueue<TcpMessage>();
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
	  if (m_pTcpMessages->at(x).pSocket == quitter) {
		 m_pTcpMessages->removeAt(x);
		 break;
	  }
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

   /**
	* Initiate communication protocol:
	*
	* Protocol:
	* =========
	*  1. Client says "HELLO\r\n".
	*  2. We respond with "HELLO\r\n".
	*  3. They identify their request,
	*     as either "REQUEST_CLIENT\r\n"
	*     or as "REQUEST_WORKER\r\n".
	*  4. We then enqueue them, and
	*     we wait for pair.
	*  5. Lastly, we send details.
	*  6. Finally, we say "BYE\r\n".
	*/
   
   QString text; TcpMessage toEnqueue;
   states state = hello;

   for (;pClientSocket->canReadLine();) {
		 if (state == states::hello) {
			QByteArray bae = pClientSocket->readLine();
			QString temp = QString(bae);
			text += temp.replace("\r\n", "");
			if (text == "HELLO") state = states::wait_request;
			else state = reject;
			std::cout<<"Read: \""<<text.toStdString()<<"\""<<std::endl;
		 } else if (state == states::wait_request) {
			QByteArray bae = pClientSocket->readLine();
			QString temp = QString(bae);
			text += temp.replace("\r\n", "");
			if (text == "REQUEST_CLIENT") {
			   /**
				* @todo handle client connection
				*/
			   std::cerr<<"client identified"<<std::endl;
			   state = reject;
			} else if (text == "REQUEST_WORKER") {
			   /**
				* @todo handle worker connection
				*/
			   std::cerr<<"worker identified"<<std::endl;
			   state = reject;
			} else state = reject;
			std::cout<<"Read: \""<<text.toStdString()<<"\""<<std::endl;
		 } else if (state == states::reject) {
			pClientSocket->write(QString("BYE\r\n").toUtf8());
			return;
		 }
   }

   // toEnqueue.line = text;
   // toEnqueue.pSocket = pClientSocket;
   // m_pTcpMessages->enqueue(toEnqueue);
   Q_EMIT receivedMessage();
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
