#ifndef __CLIENT_CONNECTION_HPP__
#define __CLIENT_CONNECTION_HPP__
#include <QtNetwork>
#include <QtCore>

#include "worker_connection.hpp"

class client_connection : public QObject
{
   Q_OBJECT
public:
   explicit client_connection(QString & _hostname,
							  QTcpSocket * _pSocket,
							  QObject * pParent = NULL
	  );

private:
   QString m_hostname;
   QTcpSocket * m_pSocket;
};
#endif
