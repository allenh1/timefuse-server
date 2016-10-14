#include "tcp_thread.hpp"
/* @todo #include "User.h" */

tcp_thread::tcp_thread(
   const QString & _hostname,                /*  hostname for TCP thread construction  */
   const quint16 & _port,                    /* port on which we construct this server */
   const bool & _master_mode,                /*         are we in master mode?         */
   QObject * parent                          /*         parent of this QObject         */
   )
   : QObject(parent)
{
   m_hostname = _hostname;
   m_port = _port;
   m_master_mode = _master_mode;
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

bool tcp_thread::writeData(QByteArray data, tcp_connection * receiver)
{
   QDataStream out(&data, QIODevice::WriteOnly);
   out.setVersion(QDataStream::Qt_5_6);
   
   QTcpSocket * to_receive = (QTcpSocket*) receiver->get_socket();

   connect(to_receive, &QAbstractSocket::disconnected, to_receive, &QObject::deleteLater);

   to_receive->write(data);
   to_receive->disconnectFromHost();
	
   return true;
}

void tcp_thread::disconnected()
{
   QTcpSocket * quitter = qobject_cast<QTcpSocket *>(sender());

   std::cout<< "Client "<< QHostAddress(quitter->peerAddress().toIPv4Address()).toString().toStdString();
   std::cout<<":" << quitter->peerPort();
   std::cout<< " has left the server." << std::endl;
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

   /* this checks if we are a master or a worker */
   if (m_master_mode) {
	  if (text == "REQUEST_CLIENT") {
		 /* this is a worker */
		 worker_connection * w = new worker_connection(hostname, pClientSocket);

		 m_p_master_node->handle_worker_connect(w);

		 std::cerr<<"emmitted worker connect"<<std::endl;
	  } else if (text == "REQUEST_WORKER") {
		 /* this is a client */
		 client_connection * c = new client_connection(hostname, pClientSocket);

		 m_p_master_node->handle_client_connect(c);
	  
		 std::cerr<<"emmitted client connect"<<std::endl;
	  } else {
		 pClientSocket->write("BYE\r\n");
		 pClientSocket->disconnectFromHost();
	  }
   } else {
	  /**
	   * @todo handle various client requests
	   */
	  std::cout<<"client request: \""<<text.toStdString()<<"\""<<std::endl;
   }
}

void tcp_thread::sendMessage(QString msg, tcp_connection * request)
{
   if (!writeData(msg.toUtf8(), request)) {
	  std::cerr<<"Unable to write message \""<<msg.toStdString()<<"\" to client."<<std::endl;
   }
}

void tcp_thread::send_pair_info(tcp_connection * request)
{
   /**
    * @todo send the pair info the worker / client called request
    */
}
