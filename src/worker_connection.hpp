#ifndef __WORKER_CONNECTION_HPP__
#define __WORKER_CONNECTION_HPP__
#include <QtNetwork>
#include <QtCore>

#include "client_connection.hpp"

class worker_connection : public QObject
{
   Q_OBJECT
public:
   explicit worker_connection(QString & _hostname,
							  QTcpSocket * _pSocket,
							  QObject * pParent = NULL
	  );

private:
   QString m_hostname;
   QTcpSocket * m_pSocket;
};
#endif
