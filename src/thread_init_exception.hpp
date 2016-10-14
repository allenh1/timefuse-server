#ifndef __THREAD_INIT_EXCEPTION_HPP__
#define __THREAD_INIT_EXCEPTION_HPP__
#include <stdexcept>

class thread_init_exception : public std::exception
{
public:
   explicit thread_init_exception(const char * _msg)
	  : m_msg(_msg)
   {/* empty constructor */}   
   explicit thread_init_exception(const std::string & _msg)
	  : m_msg(_msg)
   {/* empty constructor */}  
   virtual ~thread_init_exception() throw () {/* nothing to destruct */}

   virtual const char * what() const throw () { return m_msg.c_str(); }

protected:
   std::string m_msg;
};
#endif
