// Copyright 2017 Hunter L. Allen
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

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
	void test_create();
	void test_group_create();
	void test_join_group();
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

void test_sql_queries::test_create()
{
	/* try and create a user */
	QVERIFY(m_p_worker->try_create("billy", "password123!", "billy@domain.com"));
	QVERIFY(!m_p_worker->try_create("billy", "asdf", "bibbiliybk@domain.wrong"));
	QVERIFY(!m_p_worker->try_create("billy", "", "billy@domain.com"));
	QVERIFY(!m_p_worker->try_create("billy", "password123!", ""));
	QVERIFY(!m_p_worker->try_create("", "password123!", "billy@domain.com"));
	/* remove user from database */
	QVERIFY(m_p_worker->cleanup_db_insert());
}

void test_sql_queries::test_group_create()
{
	/* try and create a group */
	QVERIFY(m_p_worker->insert_group("billy group"));
	/* try and create the same group */
	QVERIFY(!m_p_worker->insert_group("billy group"));
	/* remove the test group */
	QVERIFY(m_p_worker->cleanup_group_insert());
}

void test_sql_queries::test_join_group()
{
	/* try and create a group */
	QVERIFY(m_p_worker->insert_group("billy group"));
	/* create a user */
	QVERIFY(m_p_worker->try_create("billy", "password123!", "billy@domain.com"));
	/* add the user to the group */
	QVERIFY(m_p_worker->join_group("billy", "billy group"));
	/* verify it can't happen twice */
	QVERIFY(!m_p_worker->join_group("billy", "billy group"));
	/* verify a non-existing user can't join an existing group */
	QVERIFY(!m_p_worker->join_group("not billy", "billy group"));
	/* verify a user can't join a non-existing group */
	QVERIFY(!m_p_worker->join_group("billy", "not billy's group"));
	/* remove the test group */
	QVERIFY(m_p_worker->cleanup_user_group_insert());
}
QTEST_MAIN(test_sql_queries)
#include "testsqlqueries.moc"
