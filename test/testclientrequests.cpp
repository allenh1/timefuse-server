#include <QtTest/QtTest>
#include "../src/worker_node.hpp"

class test_client_requests: public QObject
{
	Q_OBJECT
public:
	test_client_requests(QObject * _p_parent = NULL)
		: QObject(_p_parent),
		  m_p_master(new master_node("localhost", 3224)),
		  m_p_worker(new worker_node("localhost", 3442))
		{ /* construct the test */ }
private slots:
	void test_create();
private:
	master_node * m_p_master;
	worker_node * m_p_worker;
};



void test_client_requests::test_create()
{
	/* initialize the master */
	QVERIFY(m_p_master->init());
	/* initialize the worker */
	QVERIFY(m_p_worker->init());
	/* create a new socket */
	QTcpSocket * p_client = new QTcpSocket();
	/* connect to master */
	p_client->connectToHost("localhost", 3224, QIODevice::ReadWrite);
	/* read client location */
	p_client->waitForConnected(400);
	/* verify master accepted the connection */
	QVERIFY(p_client->state() != QAbstractSocket::UnconnectedState);
	/* write the message to the master */
	std::cout<<"**** Attempting to connect to master. ****"<<std::endl;
	p_client->write("REQUEST_WORKER\r\n"); p_client->waitForBytesWritten(-1);
	/* read from master */
	std::cout<<"**** Waiting to read from master. ****"<<std::endl;
	p_client->waitForReadyRead(); QString read = "";
	read = p_client->readLine(); read.replace("\r\n", "");
	std::cout<<"**** Read \""<<read.toStdString()<<"\" from master. ****"<<std::endl;
	QStringList split = read.split(":");
	/* parse out the location of the returned worker */
	/* verify we split the string */
	QVERIFY(split.size() > 0);
	QString host = split[0], port = split[1];
	/* verify the host is correct */
	QVERIFY(host == "localhost");
	/* verify the port is correct */
	QVERIFY(port == "3442");
	m_p_master->stop(); m_p_worker->stop();
	delete m_p_master; delete m_p_worker;
}

QTEST_MAIN(test_client_requests)
#include "testclientrequests.moc"
