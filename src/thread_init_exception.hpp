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

#ifndef __THREAD_INIT_EXCEPTION_HPP__
#define __THREAD_INIT_EXCEPTION_HPP__
#include <stdexcept>

class thread_init_exception : public std::exception
{
public:
  explicit thread_init_exception(const char * _msg)
  : m_msg(_msg)
  { /* empty constructor */}
  explicit thread_init_exception(const std::string & _msg)
  : m_msg(_msg)
  { /* empty constructor */}
  virtual ~thread_init_exception() throw () { /* nothing to destruct */}

  virtual const char * what() const throw () {return m_msg.c_str();}

protected:
  std::string m_msg;
};
#endif
