#include <QtTest/QtTest>
#include "../src/worker_node.hpp"

class test_sql_queries: public QObject
{
   Q_OBJECT
private slots:
   void test_insert();
   void test_select();
};

void test_sql_queries::test_insert()
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

   QVERIFY(worker.insert_user(billy));
   QVERIFY(worker.cleanup_db_insert());
}

void test_sql_queries::test_select()
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

   QVERIFY(worker.insert_user(billy));

   user found_billy;
   found_billy.set_username("billy");
   found_billy.set_password("password123!");

   QVERIFY(worker.select_user(found_billy));
   QVERIFY(found_billy.get_email() == billy.get_email());
   QVERIFY(found_billy.get_user_id() == billy.get_user_id());
}

QTEST_MAIN(test_sql_queries)
#include "testsqlqueries.moc"
