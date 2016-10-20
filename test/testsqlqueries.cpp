#include <QtTest/QtTest>
#include "../src/worker_node.hpp"

class test_sql_queries: public QObject
{
   Q_OBJECT
public:
   test_sql_queries(QObject * _p_parent = NULL)
	  : QObject(_p_parent),
		m_p_worker(new worker_node("localhost", 3442))
	  { /* construct the test */ }
private slots:
   void test_insert();
   void test_select();
   void test_login();
private:
   worker_node * m_p_worker;
};

void test_sql_queries::test_insert()
{
   user billy;

   billy.set_email("billy@domain.com");
   billy.set_username("billy");
   billy.set_password("password123!");

   QVERIFY(m_p_worker->insert_user(billy));
   QVERIFY(m_p_worker->cleanup_db_insert());
}

void test_sql_queries::test_select()
{
   user billy;

   billy.set_email("billy@domain.com");
   billy.set_username("billy");
   billy.set_password("password123!");

   QVERIFY(m_p_worker->insert_user(billy));

   user found_billy;
   found_billy.set_username("billy");
   found_billy.set_password("password123!");

   QVERIFY(m_p_worker->select_user(found_billy));
   QVERIFY(found_billy.get_email() == billy.get_email());
   QVERIFY(found_billy.get_username() == "billy");
   QVERIFY(found_billy.get_password() == "password123!");
   
   QVERIFY(m_p_worker->cleanup_db_insert());
}

void test_sql_queries::test_login()
{
   user billy;

   billy.set_email("billy@domain.com");
   billy.set_username("billy");
   billy.set_password("password123!");

   /* insert the user */
   QVERIFY(m_p_worker->insert_user(billy));
   /* try to login with the proper credentials */
   QVERIFY(m_p_worker->try_login("billy", "password123!"));
   QVERIFY(!m_p_worker->try_login("fake", "morefake"));
   QVERIFY(!m_p_worker->try_login("billy", "morefake"));
   QVERIFY(!m_p_worker->try_login("fake", "password123!"));
   
   /* remove user from database */
   QVERIFY(m_p_worker->cleanup_db_insert());
}
QTEST_MAIN(test_sql_queries)
#include "testsqlqueries.moc"
