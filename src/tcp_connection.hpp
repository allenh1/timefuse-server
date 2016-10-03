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
   
   virtual void disconnect();
   virtual const QString & get_hostname() { return m_hostname; }
   virtual const QTcpSocket *& get_socket() { return m_p_socket; }
private:
   QString m_hostname;
   QTcpSocket * m_p_socket;
};
#endif
