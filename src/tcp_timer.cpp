#include "tcp_timer.hpp"

tcp_timer::tcp_timer(size_t _seconds, QObject * _p_parent)
	: QObject(_p_parent),
	  m_seconds(_seconds),
	  m_p_thread(new QThread)
{ /* constructor */ }

tcp_timer::~tcp_timer()
{ /* destructor */ }

bool tcp_timer::init()
{
	this->moveToThread(m_p_thread);

	connect(m_p_thread, &QThread::started,
			this, &tcp_timer::run, Qt::DirectConnection);
	m_p_thread->start();
	return m_p_thread->isRunning();
}

void tcp_timer::run()
{
	std::cerr<<"tcp_timer running..."<<std::endl;
	m_p_thread->sleep(m_seconds);
	std::cerr<<"tcp_timer timed out!"<<std::endl;
	Q_EMIT(timeout());
}
