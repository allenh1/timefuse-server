#include <QtTest/QtTest>
#include "../src/worker_node.hpp"
#include "../src/worker_node.cpp"
#include "../src/tcp_thread.hpp"
#include "../src/tcp_thread.cpp"

class TestQString: public QObject
{
   Q_OBJECT
private slots:
   void toUpper();
};

void TestQString::toUpper()
{
   QString hostname = "localhost";
   quint16 port = 3442;
   
   worker_node worker(hostname, port);
   user billy;

   billy.set_email("billy@domain.com");
   billy.set_username("billy");
   billy.set_password("password123!");
   billy.set_user_id("-1");
   billy.set_schedule_id("-1");

   QVERIFY(worker.insert_query(billy));
}

QTEST_MAIN(TestQString)
#include "testsqlqueries.moc"
