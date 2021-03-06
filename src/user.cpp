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

#include "user.hpp"

/**
 * Constructor for a user
 *
 * @param _p_parent Parent QObject
 */
user::user(QObject * _p_parent)
: QObject(_p_parent)
{ /* Constructs a user */}

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
  if (b != (QByteArray)NULL) {
    QString temp(b);
    const QString & pipe = "|";

    QStringList parsed = temp.split(pipe);

    if (parsed.size() != 4) {
      throw std::invalid_argument("Unrecognized QByteArray syntax");
    }
    m_email = parsed[0];
    m_username_hash = parsed[1];
    m_password_hash = parsed[2];
    m_cell_phone = parsed[3];
  } else {
    throw std::invalid_argument("QByteArray b is NULL");
  }

}

user::user(const user & other)
{
  this->m_email = other.m_email;
  this->m_username_hash = other.m_username_hash;
  this->m_password_hash = other.m_password_hash;
  this->m_cell_phone = other.m_cell_phone;
  this->m_user_id = other.m_user_id;
  this->m_schedule_id = other.m_schedule_id;
}

/**
 * Destructor for a user.
 */
user::~user()
{ /* Nothing to do as of now */}

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
