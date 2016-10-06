#include "user.hpp"

/**
 * Constructor for a user
 * 
 * @param _p_parent Parent QObject
 */
user::user(QObject * _p_parent)
   : QObject(_p_parent)
{ /* Constructs a user */ }

/** 
 * Construct a user from a bytearray.
 * 
 * @param b ByteArray from a Tcp connection.
 * @param _p_parent Parent QObject
 */
user::user(const QByteArray & b, QObject * _p_parent)
   : QObject(_p_parent)
{
   /**
	* @todo parse through byte array and extract data
	*/
}

/** 
 * Destructor for a user.
 */
user::~user()
{ /* Nothing to do as of now */ }

/** 
 * @brief Convert a user to a byte array.
 * 
 * This function encodes a user to be sent
 * in a byte array to/from a client.
 *
 * @return QByteArray representing this user.
 */
QByteArray user::to_byte_array()
{
   QByteArray to_return = m_username_hash.toUtf8() + "|";
   to_return += m_password_hash.toUtf8() + "|" + m_email.toUtf8();
   to_return += "|" + m_cell_phone.toUtf8();

   return to_return;
}