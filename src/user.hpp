#ifndef __USER_HPP__
#define __USER_HPP__
#include <QtNetwork>
#include <QtCore>

class user : public QObject
{
   Q_OBJECT
public:
   explicit user();
   virtual ~user();

   const QString & get_email() { return m_email; }
   const QString & get_username() { return m_username_hash; }
   const QString & get_password() { return m_password_hash; }
   const QString & get_cell() { return m_cell_phone; }

   void set_email(const QString & _email) { m_email = _email; }
   void set_username(const QString & _username_hash);
   void set_password(const QString & _password_hash);
   void set_cell(const QString & _cell){ m_cell_phone = _cell; }
private:
   QString m_email;
   QString m_username_hash;
   QString m_password_hash;
   QString m_cell_phone;
};
#endif
