#ifndef __TCP_CONNECITON_HPP__
#define __TCP_CONNECTION_HPP__
#include <QtNetwork>
#include <QtCore>

class tcp_connection : public QObject
{
   Q_OBJECT
public:
   explicit tcp_connection(QString & _hostname,
						   QTcpSocket * _p_socket,
						   QObject * pParent = NULL
	  );
   virtual ~tcp_connection();
   
   Q_SLOT void disconnect(); /* @todo replace disconnect with a lambda */
   
   const QString & get_hostname() { return m_hostname; }
   const QTcpSocket *& get_socket() { return m_p_socket; }  
private:
   QString m_hostname;
   QTcpSocket * m_p_socket;
};
#endif
